#include "VulkanRenderBuffer.hpp"
#include "Utils/VulkanUtils.hpp"
#include "Vega/Core/Assert.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/Renderer/RenderBuffer.hpp"
#include "Vega/Utils/Log.hpp"
#include "VulkanRendererBackend.hpp"

namespace Vega
{

    VulkanRenderBuffer::VulkanRenderBuffer(const RenderBufferProps& _Props) : RenderBuffer(_Props)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        const VulkanContext& context = rendererBackend->GetVkContext();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        m_VkRenderBufferInfo = GetVulkanRenderBufferInfoByType(_Props.Type);

        VkBufferCreateInfo bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = _Props.ElementSize * _Props.ElementCount,
            .usage = m_VkRenderBufferInfo.Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VK_CHECK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, context.VkAllocator, &m_VkBuffer));

        vkGetBufferMemoryRequirements(logicalDevice, m_VkBuffer, &m_MemoryRequirements);
        uint32_t memoryTypeIndex = rendererBackend->GetVkDeviceWrapper().GetMemoryTypeIndex(
            m_MemoryRequirements.memoryTypeBits, m_VkRenderBufferInfo.MemoryProperties);
        VkMemoryAllocateInfo memoryAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = m_MemoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };

        VkResult allocateResult =
            vkAllocateMemory(logicalDevice, &memoryAllocateInfo, context.VkAllocator, &m_BufferMemory);
        if (!VulkanResultIsSuccess(allocateResult))
        {
            VEGA_CORE_ERROR("Failed to allocate memory for render buffer: {}",
                            VulkanResultString(allocateResult, true));
            VEGA_CORE_ASSERT(false, "Failed to allocate memory for render buffer");
            return;
        }

        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_DEVICE_MEMORY,
                                 m_BufferMemory, _Props.Name.data());

        VK_CHECK(vkBindBufferMemory(logicalDevice, m_VkBuffer, m_BufferMemory, 0));
    }

    void VulkanRenderBuffer::Bind(size_t _Offset)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkCommandBuffer commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();
        if (m_RenderBufferProps.Type == RenderBufferType::kVertex)
        {
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VkBuffer, reinterpret_cast<VkDeviceSize*>(&_Offset));
        }
        else if (m_RenderBufferProps.Type == RenderBufferType::kIndex)
        {
            // TODO: Support different index types
            vkCmdBindIndexBuffer(commandBuffer, m_VkBuffer, _Offset, VK_INDEX_TYPE_UINT32);
        }
        else
        {
            VEGA_CORE_ASSERT(false, "Binding is only supported for Vertex and Index RenderBufferTypes!");
        }
    }

    void VulkanRenderBuffer::DestroyInternal()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        VK_CHECK(vkDeviceWaitIdle(logicalDevice));

        vkFreeMemory(logicalDevice, m_BufferMemory, vkAllocator);
        vkDestroyBuffer(logicalDevice, m_VkBuffer, vkAllocator);
    }

    void VulkanRenderBuffer::LoadRangeInternal(size_t _Offset, size_t _Size, const void* _Data,
                                               bool _IncludeInFrameWorkload)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        if (IsVulkanRenderBufferDeviceLocal() && !IsVulkanRenderBufferHostVisible())
        {
            Ref<VulkanRenderBuffer> stagingBuffer = rendererBackend->GetCurrentStagingBuffer();
            size_t stagingOffset = stagingBuffer->LoadRange(_Size, _Data, _IncludeInFrameWorkload);
            VEGA_CORE_INFO("CopyRangeInternal: stagingOffset={}, size={}, _Offset={}", stagingOffset, _Size, _Offset);
            CopyRangeInternal(stagingOffset, stagingBuffer->GetVkBuffer(), _Offset, _Size, _IncludeInFrameWorkload);
        }
        else
        {
            void* mappedData;
            vkMapMemory(logicalDevice, m_BufferMemory, _Offset, _Size, 0, &mappedData);
            std::memcpy(mappedData, _Data, _Size);
            vkUnmapMemory(logicalDevice, m_BufferMemory);
        }
    }

    void VulkanRenderBuffer::CopyRangeInternal(size_t _SrcOffset, VkBuffer _SrcBuffer, size_t _DstOffset, size_t _Size,
                                               bool _IncludeInFrameWorkload)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VkCommandBuffer commandBuffer;
        if (!_IncludeInFrameWorkload)
        {
            // TODO: Optimize wait idle (maybe we can remove it at all)
            VK_CHECK(vkQueueWaitIdle(rendererBackend->GetVkDeviceWrapper().GetGraphicsQueue()));
            commandBuffer = rendererBackend->CreateAndBeginSingleUseCommandBuffer();
        }
        else
        {
            commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();
        }

        VkBufferCopy copyRegion = {
            .srcOffset = _SrcOffset,
            .dstOffset = _DstOffset,
            .size = _Size,
        };
        VEGA_CORE_INFO("Copying buffer range: srcOffset={}, dstOffset={}, size={}", _SrcOffset, _DstOffset, _Size);
        vkCmdCopyBuffer(commandBuffer, _SrcBuffer, m_VkBuffer, 1, &copyRegion);

        if (!_IncludeInFrameWorkload)
        {
            rendererBackend->DestroyAndEndSingleUseCommandBuffer(
                commandBuffer, rendererBackend->GetVkDeviceWrapper().GetGraphicsQueue(),
                rendererBackend->GetVkDeviceWrapper().GetGraphicsCommandPool());
        }
        else
        {
            VkMemoryBarrier memoryBarrier = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT |
                                 VK_ACCESS_TRANSFER_READ_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT |
                                 VK_ACCESS_TRANSFER_READ_BIT,
            };

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                                 &memoryBarrier, 0, nullptr, 0, nullptr);
        }
    }

    VulkanRenderBufferInfoByType VulkanRenderBuffer::GetVulkanRenderBufferInfoByType(RenderBufferType _Type)
    {
        switch (_Type)
        {
            case RenderBufferType::kVertex: {
                return {
                    .Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                };
            }
            case RenderBufferType::kIndex: {
                return {
                    .Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                };
            }
            case RenderBufferType::kUniform: {
                // TODO: check if device local and host visible is supported
                bool isSupportsDeviceLocalHostVisible = false;
                return {
                    .Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    .MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                        (isSupportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                                                          : VkMemoryPropertyFlags { 0 }),
                };
            }
            case RenderBufferType::kStaging: {
                return {
                    .Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };
            }
            case RenderBufferType::kRead: {
                return {
                    .Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    .MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };
            }
            case RenderBufferType::kStorage: {
                VEGA_CORE_ASSERT(false, "Storage RenderBufferType is not supported yet!");
                return {};
            }
            default: VEGA_CORE_ASSERT(false, "Unsupported RenderBufferType!"); return {};
        }
    }

    bool VulkanRenderBuffer::IsVulkanRenderBufferHostVisible()
    {
        return (m_VkRenderBufferInfo.MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    bool VulkanRenderBuffer::IsVulkanRenderBufferDeviceLocal()
    {
        return (m_VkRenderBufferInfo.MemoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    bool VulkanRenderBuffer::IsVulkanRenderBufferHostCoherent()
    {
        return (m_VkRenderBufferInfo.MemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ==
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

}    // namespace Vega
