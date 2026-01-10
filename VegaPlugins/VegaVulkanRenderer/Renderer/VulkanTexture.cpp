#include "VulkanTexture.hpp"

#include "Vega/Core/Assert.hpp"

#include "Utils/VulkanUtils.hpp"
#include "VulkanBase.hpp"
#include "VulkanRenderBuffer.hpp"
#include "VulkanRendererBackend.hpp"
#include "backends/imgui_impl_vulkan.h"
#include <format>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    static VkFormat ChannelsCountToVkFormat(uint32_t _ChannelCount, VkFormat _DefaultFormat);

    static VkImageType TextureTypeToVkImageType(TextureProps::TextureType _Type);

    static VkImageViewType TextureTypeToVkImageViewType(TextureProps::TextureType _Type);

    void VulkanTexture::VulkanCreate(std::string_view _Name, const TextureProps& _Props, VkFormat _Format,
                                     VkImageTiling _Tiling, VkImageUsageFlags _Usage,
                                     VkMemoryPropertyFlags _MemoryFlags, VkImageAspectFlags _ViewAspectFlags)
    {
        m_Props = _Props;

        m_ImageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = TextureTypeToVkImageType(_Props.Type),
            .format = _Format,
            .extent = { .width = _Props.Width, .height = _Props.Height, .depth = 1 },
            .mipLevels = _Props.MipLevels,
            .arrayLayers = _Props.ArraySize,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = _Tiling,
            .usage = _Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        if (_Props.Type == TextureProps::TextureType::kCubeMap)
        {
            m_ImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        m_ImageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = nullptr,
            .viewType = TextureTypeToVkImageViewType(_Props.Type),
            .format = _Format,
            .subresourceRange = {
                .aspectMask = _ViewAspectFlags,
                .baseMipLevel = 0,
                .levelCount = _Props.MipLevels,
                .baseArrayLayer = 0,
                .layerCount = _Props.ArraySize,
            },
        };

        if (_Props.ArraySize > 1)
        {
            m_ImageArrayViewsInfos.resize(_Props.ArraySize);
            m_ImageArrayViewsSubresourceRanges.resize(_Props.ArraySize);
            m_ImageArrayViews.resize(_Props.ArraySize);
            TextureProps::TextureType viewType = TextureProps::TextureType::k2D;

            for (uint32_t i = 0; i < _Props.ArraySize; i++)
            {
                m_ImageArrayViewsSubresourceRanges[i] = {
                    .aspectMask = _ViewAspectFlags,
                    .baseMipLevel = 0,
                    .levelCount = _Props.MipLevels,
                    .baseArrayLayer = i,
                    .layerCount = 1,
                };

                m_ImageArrayViewsInfos[i] = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = nullptr,
                    .viewType = TextureTypeToVkImageViewType(viewType),
                    .format = _Format,
                    .subresourceRange = m_ImageArrayViewsSubresourceRanges[i],
                };
            }
        }

        VulkanCreate(_Name);
    }

    void VulkanTexture::VulkanCreate(std::string_view _Name)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        const VulkanContext& context = rendererBackend->GetVkContext();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        VK_CHECK(vkCreateImage(logicalDevice, &m_ImageInfo, context.VkAllocator, &m_Image));

        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_IMAGE, m_Image,
                                 _Name.data());

        vkGetImageMemoryRequirements(logicalDevice, m_Image, &m_MemoryRequirements);

        uint32_t memoryTypeIndex = rendererBackend->GetVkDeviceWrapper().GetMemoryTypeIndex(
            m_MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo memoryAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = m_MemoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };
        VkResult allocateResult =
            vkAllocateMemory(logicalDevice, &memoryAllocateInfo, context.VkAllocator, &m_ImageMemory);
        if (!VulkanResultIsSuccess(allocateResult))
        {
            VEGA_CORE_ERROR("Failed to allocate memory for image: {}", VulkanResultString(allocateResult, true));
            VEGA_CORE_ASSERT(false, "Failed to allocate memory for image");
            return;
        }

        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_DEVICE_MEMORY,
                                 m_ImageMemory, _Name.data());

        VK_CHECK(vkBindImageMemory(logicalDevice, m_Image, m_ImageMemory, 0));

        m_CurrentLayout = m_ImageInfo.initialLayout;

        m_ImageViewInfo.image = m_Image;
        VK_CHECK(vkCreateImageView(logicalDevice, &m_ImageViewInfo, context.VkAllocator, &m_ImageView));
        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_IMAGE_VIEW,
                                 m_ImageView, std::format("{}_view", _Name.data()).c_str());

        if (m_Props.ArraySize > 1)
        {
            for (uint32_t i = 0; i < m_Props.ArraySize; i++)
            {
                m_ImageArrayViewsInfos[i].image = m_Image;
                VK_CHECK(vkCreateImageView(logicalDevice, &m_ImageArrayViewsInfos[i], context.VkAllocator,
                                           &m_ImageArrayViews[i]));
                VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                                         VK_OBJECT_TYPE_IMAGE_VIEW, m_ImageArrayViews[i],
                                         std::format("{}_view_layer_{}", _Name.data(), i).c_str());
            }
        }

        if (m_Props.IsUsedForGui)
        {
            VkSamplerCreateInfo samplerInfo {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .maxAnisotropy = 1.0f,
                .minLod = -1000,
                .maxLod = 1000,
            };
            VK_CHECK(vkCreateSampler(logicalDevice, &samplerInfo, context.VkAllocator, &m_Sampler));

            m_DescriptorSet =
                ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    VulkanTextureTransitionMaskResult VulkanTexture::GetTransitionMask(VkImageLayout _OldLayout,
                                                                       VkImageLayout _NewLayout)
    {
        if (_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            return {
                .SrcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .SrcAccessMask = 0,
                .DstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            };
        }
        if (_OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            _NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            return {
                .SrcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .SrcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .DstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .DstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            };
        }
        if (_OldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
            _NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            return {
                .SrcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .SrcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                .DstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .DstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            };
        }
        if (_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _NewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            return {
                .SrcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .SrcAccessMask = 0,
                .DstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .DstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            };
        }
        if (_OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            _NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            return {
                .SrcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .SrcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .DstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            };
        }
        if (_OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
            _NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            return {
                .SrcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .DstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .DstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            };
            // return {
            //     .SrcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            //     .SrcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            //     .DstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            //     .DstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            // };
        }

        VEGA_CORE_ASSERT(false, "Unsupported layout transition!");
        return {};
    }

    void VulkanTexture::TransitionImageLayout(VkImageLayout _OldLayout, VkImageLayout _NewLayout,
                                              VkCommandBuffer _CommandBuffer)
    {
        VulkanTextureTransitionMaskResult transitionMask = GetTransitionMask(_OldLayout, _NewLayout);

        VkImageSubresourceRange imageSubresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        // VkImageMemoryBarrier barrierToColor {
        //     .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        //     .srcAccessMask = 0,
        //     .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        //     .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,    // только при первом кадре
        //     .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        //     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        //     .image = m_VulkanTextures[0][rendererBackend->GetCurrentImageIndex()]->GetTextureVkImage(),
        //     .subresourceRange = imageSubresourceRange,
        // };
        //
        // vkCmdPipelineBarrier(rendererBackend->GetCurrentGraphicsCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        //                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
        //                      &barrierToColor);

        VkImageMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

            .srcStageMask = transitionMask.SrcStageMask,
            .srcAccessMask = transitionMask.SrcAccessMask,

            .dstStageMask = transitionMask.DstStageMask,
            .dstAccessMask = transitionMask.DstAccessMask,

            .oldLayout = _OldLayout,
            .newLayout = _NewLayout,

            .image = m_Image,
            .subresourceRange = imageSubresourceRange,
        };

        VkDependencyInfo depInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        };

        vkCmdPipelineBarrier2(_CommandBuffer, &depInfo);

        m_CurrentLayout = _NewLayout;
    }

    void VulkanTexture::TransitionImageLayout(VkImageLayout _NewLayout, VkCommandBuffer _CommandBuffer)
    {
        if (m_CurrentLayout == _NewLayout)
        {
            return;
        }
        TransitionImageLayout(m_CurrentLayout, _NewLayout, _CommandBuffer);
    }

    void VulkanTexture::Create(std::string_view _Name, const TextureProps& _Props)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VkImageUsageFlags usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageAspectFlags aspect;
        VkFormat imageFormat;
        if (_Props.Flags & TextureProps::kDepthAttachment)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageFormat = rendererBackend->GetVkDeviceWrapper().GetDepthFormat();
        }
        else
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            imageFormat = ChannelsCountToVkFormat(_Props.ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);
        }

        VulkanCreate(_Name, _Props, imageFormat, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     aspect);
    }

    void VulkanTexture::Create(std::string_view _Name, const TextureProps& _Props, uint8_t* _Data)
    {
        Create(_Name, _Props);
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VkCommandBuffer uploadCommandBuffer = rendererBackend->CreateAndBeginSingleUseCommandBuffer();

        size_t bufferSize = static_cast<VkDeviceSize>(_Props.Width * _Props.Height * _Props.ChannelCount);

        // TODO: Move staging buffer creation to VulkanCommandBuffer class or to renderer backend
        // TODO: Create a staging buffer pool to avoid creating and destroying buffers every time

        VulkanRenderBuffer stagingBuffer(RenderBufferProps {
            .Name = std::format("VulkanTexture_{}_staging_buffer", _Name),
            .Type = RenderBufferType::kStaging,
            .ElementSize = 1,
            .ElementCount = bufferSize,
        });

        // TODO: May need to add IncludeInFrameWorkload parameter
        stagingBuffer.LoadRange(bufferSize, _Data, false);

        TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uploadCommandBuffer);

        // TODO: Move copy command to renderer backend

        VkBufferImageCopy region = {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = { .x = 0, .y = 0, .z = 0 },
                .imageExtent = { .width = _Props.Width, .height = _Props.Height, .depth = 1 }
            };

        vkCmdCopyBufferToImage(uploadCommandBuffer, stagingBuffer.GetVkBuffer(), m_Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              uploadCommandBuffer);

        rendererBackend->DestroyAndEndSingleUseCommandBuffer(
            uploadCommandBuffer, rendererBackend->GetVkDeviceWrapper().GetGraphicsQueue(),
            rendererBackend->GetVkDeviceWrapper().GetGraphicsCommandPool());

        stagingBuffer.Destroy();
    }

    void VulkanTexture::CreateSwapchainTexture(VkImage _Image, VkSurfaceFormatKHR _ImageFormat,
                                               const TextureProps& _Props)
    {
        m_Props = _Props;
        // _ImageFormat.format;

        m_Image = _Image;

        m_ImageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_Image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = _ImageFormat.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VK_CHECK(vkCreateImageView(rendererBackend->GetVkDeviceWrapper().GetLogicalDevice(), &m_ImageViewInfo,
                                   rendererBackend->GetVkContext().VkAllocator, &m_ImageView));
    }

    void VulkanTexture::Destroy()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        // TODO: find a better way to wait for all operations to be done on the texture
        VK_CHECK(vkDeviceWaitIdle(logicalDevice));

        if (m_ImageView)
        {
            vkDestroyImageView(logicalDevice, m_ImageView, vkAllocator);

            for (VkImageView imageView : m_ImageArrayViews)
            {
                vkDestroyImageView(logicalDevice, imageView, vkAllocator);
            }

            m_ImageView = nullptr;
            m_ImageArrayViews.clear();
            m_ImageArrayViewsSubresourceRanges.clear();
            m_ImageArrayViewsInfos.clear();
        }

        vkFreeMemory(logicalDevice, m_ImageMemory, vkAllocator);
        m_ImageMemory = nullptr;

        vkDestroyImage(logicalDevice, m_Image, vkAllocator);
        m_Image = nullptr;

        if (m_DescriptorSet != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(m_DescriptorSet);
        }
        m_DescriptorSet = VK_NULL_HANDLE;
        if (m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(logicalDevice, m_Sampler, vkAllocator);
        }
        m_Sampler = VK_NULL_HANDLE;
    }

    void VulkanTexture::Resize(std::string_view _Name, uint32_t _NewWidth, uint32_t _NewHeight)
    {
        m_Props.Width = _NewWidth;
        m_Props.Height = _NewHeight;

        m_ImageInfo.extent.width = _NewWidth;
        m_ImageInfo.extent.height = _NewHeight;

        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        vkDestroyImage(logicalDevice, m_Image, vkAllocator);
        vkFreeMemory(logicalDevice, m_ImageMemory, vkAllocator);

        if (m_ImageView)
        {
            vkDestroyImageView(logicalDevice, m_ImageView, vkAllocator);

            for (VkImageView imageView : m_ImageArrayViews)
            {
                vkDestroyImageView(logicalDevice, imageView, vkAllocator);
            }
        }

        VulkanCreate(_Name);
    }

    void VulkanTexture::ClearColor(const glm::vec4& _ClearColor)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkCommandBuffer commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();

        TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer);

        VkClearColorValue clearColor = {
            .float32 = { _ClearColor.r, _ClearColor.g, _ClearColor.b, _ClearColor.a }
        };

        vkCmdClearColorImage(
            commandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, m_Props.ArraySize,
            m_Props.ArraySize == 1 ? &m_ImageViewInfo.subresourceRange : m_ImageArrayViewsSubresourceRanges.data());

        TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              commandBuffer);
    }

    void VulkanTexture::ClearDepthStencil()
    {
        VEGA_CORE_ASSERT(false, "TODO: Implement VulkanTexture::ClearDepthStencil");
    }

    void* VulkanTexture::GetTextureGuiId() const { return reinterpret_cast<void*>(m_DescriptorSet); }

    static VkFormat ChannelsCountToVkFormat(uint32_t _ChannelCount, VkFormat _DefaultFormat)
    {
        switch (_ChannelCount)
        {
            case 1: return VK_FORMAT_R8_UNORM;
            case 2: return VK_FORMAT_R8G8_UNORM;
            case 3: return VK_FORMAT_R8G8B8_UNORM;
            case 4:
                return VK_FORMAT_B8G8R8A8_UNORM;
                // case 4: return VK_FORMAT_R8G8B8A8_UNORM;
        }

        VEGA_CORE_ASSERT(false, "ChannelsCountToVkFormat: Unknown ChannelCount!")

        return _DefaultFormat;
    }

    static VkImageType TextureTypeToVkImageType(TextureProps::TextureType _Type)
    {
        switch (_Type)
        {
            case TextureProps::TextureType::k2D: return VK_IMAGE_TYPE_2D;
            case TextureProps::TextureType::kCubeMap: return VK_IMAGE_TYPE_2D;
            case TextureProps::TextureType::k3D: return VK_IMAGE_TYPE_3D;
            case TextureProps::TextureType::kArray: return VK_IMAGE_TYPE_2D;
        }

        VEGA_CORE_ASSERT(false, "TextureTypeToVkImageType: Unknown TextureType!")
        return VK_IMAGE_TYPE_2D;
    }

    static VkImageViewType TextureTypeToVkImageViewType(TextureProps::TextureType _Type)
    {
        switch (_Type)
        {
            case TextureProps::TextureType::k2D: return VK_IMAGE_VIEW_TYPE_2D;
            case TextureProps::TextureType::kCubeMap: return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureProps::TextureType::k3D: return VK_IMAGE_VIEW_TYPE_3D;
            case TextureProps::TextureType::kArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }

        VEGA_CORE_ASSERT(false, "TextureTypeToVkImageViewType: Unknown TextureType!")
        return VK_IMAGE_VIEW_TYPE_2D;
    }

}    // namespace Vega
