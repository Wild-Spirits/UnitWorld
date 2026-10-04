#include "VulkanSampler.hpp"

#include "Utils/VulkanUtils.hpp"
#include "VulkanRendererBackend.hpp"

namespace Vega
{

    VulkanSampler::VulkanSampler(std::string_view _Name, const SamplerProps& _Props) : m_Props(_Props)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        const VulkanContext& context = rendererBackend->GetVkContext();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        // TODO: Use SamplerProps
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
        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_SAMPLER,
                                 m_Sampler, _Name.data());
    }

    void VulkanSampler::OnDetach()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        const VulkanContext& context = rendererBackend->GetVkContext();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        if (m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(logicalDevice, m_Sampler, context.VkAllocator);
        }
        m_Sampler = VK_NULL_HANDLE;
    }

}    // namespace Vega
