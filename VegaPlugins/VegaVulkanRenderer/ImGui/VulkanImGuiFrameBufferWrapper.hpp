#pragma once

#include "Renderer/VulkanFrameBuffer.hpp"
#include "Renderer/VulkanSampler.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/ImGui/ImGuiFrameBufferWrapper.hpp"
#include "VulkanImGuiTextureWrapper.hpp"

namespace Vega
{

    class VulkanImGuiFrameBufferWrapper : public ImGuiFrameBufferWrapper
    {
    public:
        VulkanImGuiFrameBufferWrapper(Ref<VulkanFrameBuffer> _FrameBuffer, Ref<VulkanSampler> _Sampler);

        void OnDetach() override;

        virtual void* GetImGuiColorAttachmentId(size_t _AttachmentIndex = 0) const override;
        virtual void* GetImGuiColorDepthAttachmentId(size_t _AttachmentIndex = 0) const override;

    protected:
        std::vector<std::vector<Ref<VulkanImGuiTextureWrapper>>> m_ColorAttachmentTextures;
        std::vector<std::vector<Ref<VulkanImGuiTextureWrapper>>> m_DepthAttachmentTextures;
    };

}    // namespace Vega
