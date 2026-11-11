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

        inline VkExtent2D GetExtent() const { return m_Extent; }

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

        VkSurfaceFormatKHR m_ImageFormat = {};

        RendererBackendConfig::Flags m_RendererFlags = 0;

        // Must stay 0 until Create succeeds: OnWindowCreate sizes per-frame containers by this value
        uint32_t m_MaxFramesInFlight = 0;

        VkExtent2D m_Extent = { 0, 0 };

        VkSwapchainKHR m_VkSwapchain = nullptr;

        std::vector<VkImage> m_SwapchainImages;

        std::vector<Ref<VulkanTexture>> m_VulkanColorBufferTextures;
        std::vector<Ref<Texture>> m_ColorBufferTextures;
    };

}    // namespace Vega
