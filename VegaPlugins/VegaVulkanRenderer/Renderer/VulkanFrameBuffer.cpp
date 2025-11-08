#include "VulkanFrameBuffer.hpp"
#include "Renderer/VulkanRendererBackend.hpp"
#include "Vega/Core/Application.hpp"
#include "Vega/Core/Window.hpp"
#include "VulkanBase.hpp"
#include "VulkanTexture.hpp"
#include "backends/imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace Vega
{

    VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferProps& _Props) : m_Props(_Props)
    {
        Ref<Window> window = Application::Get().GetWindow();

        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;
        size_t imageCount = rendererBackend->GetVkSwapchain().GetImagesCount();

        if (m_Props.IsUsedInFlight)
        {
            for (size_t i = 0; i < imageCount; ++i)
            {
                Ref<VulkanTexture> texture = CreateRef<VulkanTexture>();
                texture->Create(std::format("VulkanFrameBuffer_{}_{}", m_Props.Name, i),
                                {
                                    .Width = window->GetWidth(),
                                    .Height = window->GetHeight(),
                                    .ChannelCount = 4,
                                });

                m_VulkanTextures.push_back(texture);
                m_Textures.push_back(texture);

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
    }

    void* VulkanFrameBuffer::GetInGuiRenderId() const
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        return m_DescriptorSets[rendererBackend->GetCurrentImageIndex()];
    }

    void VulkanFrameBuffer::TransitToGui()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VkImageSubresourceRange imageSubresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,

        };

        VkImageMemoryBarrier barrierToSample = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_VulkanTextures[rendererBackend->GetCurrentImageIndex()]->GetTextureVkImage(),
            .subresourceRange = imageSubresourceRange,
        };

        vkCmdPipelineBarrier(rendererBackend->GetCurrentGraphicsCommandBuffer(),
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrierToSample);
    }

    void VulkanFrameBuffer::Bind()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VkImageSubresourceRange imageSubresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkImageMemoryBarrier barrierToColor {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,    // только при первом кадре
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_VulkanTextures[rendererBackend->GetCurrentImageIndex()]->GetTextureVkImage(),
            .subresourceRange = imageSubresourceRange,
        };

        vkCmdPipelineBarrier(rendererBackend->GetCurrentGraphicsCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrierToColor);
    }    // namespace Vega

}    // namespace Vega
