#pragma once

#include "VulkanBase.hpp"
#include "VulkanDeviceWrapper.hpp"

#include <Vega/Renderer/RendererBackendTypes.hpp>

#include "VulkanTexture.hpp"

namespace Vega
{

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain() = default;

        bool Create(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                    RendererBackendConfig::Flags _RendererFlags, VkSurfaceKHR _VkSurface);
        bool Recreate(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                      RendererBackendConfig::Flags _RendererFlags, VkSurfaceKHR _VkSurface);
        void Destroy(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper);

        inline uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

        inline size_t GetImagesCount() const { return m_SwapchainImages.size(); }

        inline const std::vector<Ref<VulkanTexture>>& GetVulkanColorTextures() const
        {
            return m_VulkanColorBufferTextures;
        }
        inline const std::vector<Ref<Texture>>& GetColorTextures() const { return m_ColorBufferTextures; }

        inline VkFormat GetImageFormat() const { return m_ImageFormat.format; }

        inline const VulkanSwapchainSupportInfo& GetSwapchainSupportInfo() const { return m_SwapchainSupportInfo; }

        inline VkSwapchainKHR GetSwapchainHandle() const { return m_VkSwapchain; }

    private:
        VulkanSwapchainSupportInfo m_SwapchainSupportInfo;

        VkSurfaceFormatKHR m_ImageFormat;

        RendererBackendConfig::Flags m_RendererFlags;

        uint32_t m_MaxFramesInFlight;

        VkSwapchainKHR m_VkSwapchain;

        std::vector<VkImage> m_SwapchainImages;

        std::vector<Ref<VulkanTexture>> m_VulkanColorBufferTextures;
        std::vector<Ref<Texture>> m_ColorBufferTextures;
    };

}    // namespace Vega
