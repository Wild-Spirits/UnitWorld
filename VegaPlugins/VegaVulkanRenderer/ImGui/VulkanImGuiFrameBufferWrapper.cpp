#include "VulkanImGuiFrameBufferWrapper.hpp"
#include "Renderer/VulkanRendererBackend.hpp"

namespace Vega
{

    VulkanImGuiFrameBufferWrapper::VulkanImGuiFrameBufferWrapper(Ref<VulkanFrameBuffer> _FrameBuffer,
                                                                 Ref<VulkanSampler> _Sampler)
    {
        const std::vector<std::vector<Ref<VulkanTexture>>>& colorAttachments = _FrameBuffer->GetVulkanColorTextures();
        m_ColorAttachmentTextures.resize(colorAttachments.size());
        for (size_t i = 0; i < colorAttachments.size(); i++)
        {
            m_ColorAttachmentTextures[i].reserve(colorAttachments[i].size());
            for (size_t j = 0; j < colorAttachments[i].size(); j++)
            {
                m_ColorAttachmentTextures[i].emplace_back(
                    CreateRef<VulkanImGuiTextureWrapper>(colorAttachments[i][j], _Sampler));
            }
        }

        const std::vector<std::vector<Ref<VulkanTexture>>>& depthAttachments = _FrameBuffer->GetVulkanDepthTextures();
        m_DepthAttachmentTextures.resize(depthAttachments.size());
        for (size_t i = 0; i < depthAttachments.size(); i++)
        {
            m_DepthAttachmentTextures[i].resize(depthAttachments[i].size());
            for (size_t j = 0; j < depthAttachments[i].size(); j++)
            {
                m_DepthAttachmentTextures[i].emplace_back(
                    CreateRef<VulkanImGuiTextureWrapper>(depthAttachments[i][j], _Sampler));
            }
        }
    }

    void VulkanImGuiFrameBufferWrapper::OnDetach()
    {
        for (auto& colorAttachmentSet : m_ColorAttachmentTextures)
        {
            for (auto& textureWrapper : colorAttachmentSet)
            {
                textureWrapper->OnDetach();
            }
        }
        for (auto& depthAttachmentSet : m_DepthAttachmentTextures)
        {
            for (auto& textureWrapper : depthAttachmentSet)
            {
                textureWrapper->OnDetach();
            }
        }

        m_ColorAttachmentTextures.clear();
        m_DepthAttachmentTextures.clear();
    }

    void* VulkanImGuiFrameBufferWrapper::GetImGuiColorAttachmentId(size_t _AttachmentIndex) const
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        // rendererBackend->GetCurrentImageIndex()
        return m_ColorAttachmentTextures[_AttachmentIndex][rendererBackend->GetCurrentImageIndex()]
            ->GetImGuiTextureId();
    }

    void* VulkanImGuiFrameBufferWrapper::GetImGuiColorDepthAttachmentId(size_t _AttachmentIndex) const
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        return m_DepthAttachmentTextures[_AttachmentIndex][rendererBackend->GetCurrentImageIndex()]
            ->GetImGuiTextureId();
    }

}    // namespace Vega
