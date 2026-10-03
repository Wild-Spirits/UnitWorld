#pragma once

#include "Renderer/VulkanTexture.hpp"
#include "Vega/Renderer/FrameBuffer.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    class VulkanFrameBuffer : public FrameBuffer
    {
    public:
        VulkanFrameBuffer(const FrameBufferProps& _Props);
        virtual ~VulkanFrameBuffer() override = default;

        const std::vector<std::vector<Ref<VulkanTexture>>>& GetVulkanColorTextures() const
        {
            return m_VulkanColorTextures;
        }
        const std::vector<std::vector<Ref<VulkanTexture>>>& GetVulkanDepthTextures() const
        {
            return m_VulkanDepthTextures;
        }

        void Create();
        void Destroy() override;

        void Resize(uint32_t _Width, uint32_t _Height) override;

        uint32_t GetWidth() const override { return m_Props.Width; }
        uint32_t GetHeight() const override { return m_Props.Height; }

        void TransitColorAttachmentToGui(size_t _AttachmentIndex = 0) override;

        void BindAndClearColorDepthStencil() override;

        void Bind() override;

    protected:
        FrameBufferProps m_Props;

        std::vector<std::vector<Ref<VulkanTexture>>> m_VulkanColorTextures;

        std::vector<std::vector<Ref<VulkanTexture>>> m_VulkanDepthTextures;
    };

}    // namespace Vega
