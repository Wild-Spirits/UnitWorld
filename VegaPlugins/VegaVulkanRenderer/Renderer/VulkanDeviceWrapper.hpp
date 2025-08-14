#pragma once

#include "VulkanBase.hpp"

namespace Vega
{

    struct VulkanSwapchainSupportInfo
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct VulkanPhysicalDeviceQueueFamilyInfo
    {
        int32_t GraphicsQueueIndex;
        int32_t PresentQueueIndex;
        int32_t ComputeQueueIndex;
        int32_t TransferQueueIndex;
    };

    class VulkanDeviceWrapper
    {
    public:
        bool Init(VulkanContext& _Context);
        void Shutdown(const VulkanContext& _Context);

        VulkanSwapchainSupportInfo GetSwapchainSupportInfo(VkSurfaceKHR _Surface) const;
        void DetectDepthFormat();

        inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
        inline const VulkanPhysicalDeviceQueueFamilyInfo& GetPhysicalDeviceQueueFamilyInfo() const
        {
            return m_PhysicalDeviceQueueFamilyInfo;
        }
        inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        inline VkQueue GetPresentQueue() const { return m_PresentQueue; }
        inline VkFormat GetDepthFormat() const { return m_DepthFormat; }
        inline uint8_t GetDepthChannelCount() const { return m_DepthChannelCount; }
        inline VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
        inline const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatures() const { return m_PhysicalDeviceFeatures; }

        uint32_t GetMemoryTypeIndex(uint32_t _TypeBits, VkMemoryPropertyFlags _Properties) const;
        inline uint32_t GetMinUniformBufferOffsetAligment() const
        {
            return m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
        }

        inline const VulkanDeviceSupportFlags GetSupportFlags() const { return m_SupportFlags; }

    protected:
        bool SelectPhysicalDevice(VkInstance _VkInstance);

    protected:
        uint32_t m_ApiMajor;
        uint32_t m_ApiMinor;
        uint32_t m_ApiPatch;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_LogicalDevice;

        VulkanPhysicalDeviceQueueFamilyInfo m_PhysicalDeviceQueueFamilyInfo;
        bool m_SupportDeviceLocalHostVisible;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
        VkQueue m_TransferQueue;
        VkQueue m_ComputeQueue;

        VkCommandPool m_GraphicsCommandPool;

        VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
        VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
        VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;

        VkFormat m_DepthFormat;
        uint8_t m_DepthChannelCount;

        VulkanDeviceSupportFlags m_SupportFlags = 0;
    };

}    // namespace Vega
