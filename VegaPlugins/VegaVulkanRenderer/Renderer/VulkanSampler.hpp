#pragma once

#include "Vega/Renderer/Sampler.hpp"

#include <string_view>

#include <vulkan/vulkan_core.h>

namespace Vega
{

    class VulkanSampler : public Sampler
    {
    public:
        VulkanSampler(std::string_view _Name, const SamplerProps& _Props);
        ~VulkanSampler() = default;

        VkSampler GetVkSampler() const { return m_Sampler; }

        void OnDetach() override;

    protected:
        SamplerProps m_Props;

        VkSampler m_Sampler;
    };

}    // namespace Vega
