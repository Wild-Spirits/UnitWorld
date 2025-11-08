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

        const std::vector<Ref<Texture>>& GetTextures() const override { return m_Textures; }

        void* GetInGuiRenderId() const override;

        void TransitToGui() override;

        void Bind() override;

    protected:
        FrameBufferProps m_Props;

        std::vector<Ref<VulkanTexture>> m_VulkanTextures;
        std::vector<Ref<Texture>> m_Textures;

        std::vector<VkSampler> m_Samplers;
        std::vector<VkDescriptorSet> m_DescriptorSets;
    };

}    // namespace Vega
