#include "VulkanFrameBuffer.hpp"

#include "Renderer/VulkanRendererBackend.hpp"
#include "VulkanBase.hpp"
#include "VulkanTexture.hpp"

#include "backends/imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace Vega
{

    VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferProps& _Props) : m_Props(_Props) { Create(); }

    void VulkanFrameBuffer::Create()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;
        size_t imageCount = rendererBackend->GetVkSwapchain().GetImagesCount();

        if (m_Props.IsUsedInFlight)
        {
            m_VulkanTextures.emplace_back();
            for (size_t i = 0; i < imageCount; ++i)
            {
                Ref<VulkanTexture> texture = CreateRef<VulkanTexture>();
                texture->Create(std::format("VulkanFrameBuffer_{}_{}", m_Props.Name, i), {
                                                                                             .Width = m_Props.Width,
                                                                                             .Height = m_Props.Height,
                                                                                             .ChannelCount = 4,
                                                                                         });

                m_VulkanTextures[0].push_back(texture);

                // TODO: Move to Texture class?
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
                    VkSampler sampler;
                    VK_CHECK(vkCreateSampler(logicalDevice, &samplerInfo, vkAllocator, &sampler));
                    m_Samplers.push_back(sampler);
                    VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
                        sampler, texture->GetTextureVkImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    m_DescriptorSets.push_back(descriptorSet);
                }
            }
        }
        // TODO: Implement else
    }

    void VulkanFrameBuffer::Destroy()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        // TODO: find a better way to wait for all operations to be done on the framebuffer
        VK_CHECK(vkDeviceWaitIdle(logicalDevice));

        for (auto& descriptorSet : m_DescriptorSets)
        {
            if (descriptorSet != VK_NULL_HANDLE)
            {
                ImGui_ImplVulkan_RemoveTexture(descriptorSet);
            }
        }
        m_DescriptorSets.clear();

        for (auto& sampler : m_Samplers)
        {
            if (sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(logicalDevice, sampler, vkAllocator);
            }
        }
        m_Samplers.clear();

        for (auto& textures : m_VulkanTextures)
        {
            for (auto& texture : textures)
            {
                texture->Destroy();
            }
        }
        m_VulkanTextures.clear();

        for (auto& textures : m_VulkanDepthTextures)
        {
            for (auto& texture : textures)
            {
                texture->Destroy();
            }
        }
        m_VulkanDepthTextures.clear();
    }

    void VulkanFrameBuffer::Resize(uint32_t _Width, uint32_t _Height)
    {
        m_Props.Width = _Width;
        m_Props.Height = _Height;

        Destroy();
        Create();
    }

    void* VulkanFrameBuffer::GetInGuiRenderId() const
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        return m_DescriptorSets[rendererBackend->GetCurrentImageIndex()];
    }

    void VulkanFrameBuffer::TransitToGui()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        m_VulkanTextures[0][rendererBackend->GetCurrentImageIndex()]->TransitionImageLayout(
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            rendererBackend->GetCurrentGraphicsCommandBuffer());
    }

    void VulkanFrameBuffer::Bind()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        m_VulkanTextures[0][rendererBackend->GetCurrentImageIndex()]->TransitionImageLayout(
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            rendererBackend->GetCurrentGraphicsCommandBuffer());
    }

}    // namespace Vega
