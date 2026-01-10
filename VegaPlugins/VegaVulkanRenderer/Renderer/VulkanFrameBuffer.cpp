#include "VulkanFrameBuffer.hpp"

#include "Renderer/VulkanRendererBackend.hpp"
#include "VulkanBase.hpp"
#include "VulkanTexture.hpp"

#include <vulkan/vulkan_core.h>

namespace Vega
{

    VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferProps& _Props) : m_Props(_Props) { Create(); }

    void VulkanFrameBuffer::Create()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        size_t imageCount = rendererBackend->GetVkSwapchain().GetImagesCount();

        if (m_Props.IsUsedInFlight)
        {
            m_VulkanTextures.emplace_back();
            for (size_t i = 0; i < imageCount; ++i)
            {
                Ref<VulkanTexture> texture = CreateRef<VulkanTexture>();
                texture->Create(std::format("VulkanFrameBuffer_{}_{}", m_Props.Name, i),
                                {
                                    .Width = m_Props.Width,
                                    .Height = m_Props.Height,
                                    .ChannelCount = 4,
                                    .IsUsedForGui = m_Props.IsUsedForGui,
                                });

                m_VulkanTextures[0].push_back(texture);
            }
        }
        // TODO: Implement else
    }

    void VulkanFrameBuffer::Destroy()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        // TODO: find a better way to wait for all operations to be done on the framebuffer
        VK_CHECK(vkDeviceWaitIdle(logicalDevice));

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

        return m_VulkanTextures[0][rendererBackend->GetCurrentImageIndex()]->GetTextureGuiId();
    }

    void VulkanFrameBuffer::TransitToGui()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        m_VulkanTextures[0][rendererBackend->GetCurrentImageIndex()]->TransitionImageLayout(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rendererBackend->GetCurrentGraphicsCommandBuffer());
    }

    void VulkanFrameBuffer::BindAndClearColorDepthStencil()
    {
        glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

        for (auto& textures : m_VulkanTextures)
        {
            for (auto& texture : textures)
            {
                texture->ClearColor(clearColor);
            }
        }

        for (auto& textures : m_VulkanDepthTextures)
        {
            for (auto& texture : textures)
            {
                texture->ClearDepthStencil();
            }
        }
    }

    void VulkanFrameBuffer::Bind()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        m_VulkanTextures[0][rendererBackend->GetCurrentImageIndex()]->TransitionImageLayout(
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, rendererBackend->GetCurrentGraphicsCommandBuffer());
    }

}    // namespace Vega
