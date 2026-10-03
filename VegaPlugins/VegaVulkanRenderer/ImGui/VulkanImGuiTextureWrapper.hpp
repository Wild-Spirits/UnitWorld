#pragma once

#include "Renderer/VulkanSampler.hpp"
#include "Renderer/VulkanTexture.hpp"
#include "Vega/ImGui/ImGuiTextureWrapper.hpp"

namespace Vega
{

    class VulkanImGuiTextureWrapper : public ImGuiTextureWrapper
    {
    public:
        VulkanImGuiTextureWrapper(Ref<VulkanTexture> _Texture, Ref<VulkanSampler> _Sampler);

        virtual void OnDetach() override;

        virtual void* GetImGuiTextureId() const override;

    protected:
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
    };

}    // namespace Vega
