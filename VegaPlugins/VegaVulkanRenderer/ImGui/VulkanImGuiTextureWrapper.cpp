#include "VulkanImGuiTextureWrapper.hpp"
#include "Renderer/VulkanRendererBackend.hpp"

#include "backends/imgui_impl_vulkan.h"

namespace Vega
{

    VulkanImGuiTextureWrapper::VulkanImGuiTextureWrapper(Ref<VulkanTexture> _Texture, Ref<VulkanSampler> _Sampler)
    {

        m_DescriptorSet = ImGui_ImplVulkan_AddTexture(_Sampler->GetVkSampler(), _Texture->GetTextureVkImageView(),
                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanImGuiTextureWrapper::OnDetach()
    {
        if (m_DescriptorSet != VK_NULL_HANDLE)
        {
            // NOTE: The descriptor set may still be used by ImGui draw commands of frames in flight
            VkDescriptorSet descriptorSet = m_DescriptorSet;
            VulkanRendererBackend::GetVkRendererBackend()->EnqueueDeletion(
                [descriptorSet]() { ImGui_ImplVulkan_RemoveTexture(descriptorSet); });
        }
        m_DescriptorSet = VK_NULL_HANDLE;
    }

    void* VulkanImGuiTextureWrapper::GetImGuiTextureId() const { return reinterpret_cast<void*>(m_DescriptorSet); }

}    // namespace Vega
