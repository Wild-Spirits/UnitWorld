#pragma once

#include "VkBase.hpp"

namespace Vega
{

    struct VkSwapchainSupportInfo
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        uint32_t FormatCount;
        VkSurfaceFormatKHR* Formats;
        uint32_t PresentModeCount;
        VkPresentModeKHR* PresentModes;
    };

    struct VkPhysicalDeviceQueueFamilyInfo
    {
        int32_t GraphicsQueueIndex;
        int32_t PresentQueueIndex;
        int32_t ComputeQueueIndex;
        int32_t TransferQueueIndex;
    };

    class VkDeviceWrapper
    {
    public:
        bool Init(VkContext& _Context);
        void Shutdown();

        VkSwapchainSupportInfo GetSwapchainSupportInfo(VkSurfaceKHR _Surface) const;

        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
        const VkPhysicalDeviceQueueFamilyInfo& GetPhysicalDeviceQueueFamilyInfo() const
        {
            return m_PhysicalDeviceQueueFamilyInfo;
        }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

    protected:
        bool SelectPhysicalDevice(VkInstance _VkInstance);

    protected:
        uint32_t m_ApiMajor;
        uint32_t m_ApiMinor;
        uint32_t m_ApiPatch;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_LogicalDevice;

        VkPhysicalDeviceQueueFamilyInfo m_PhysicalDeviceQueueFamilyInfo;
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

        uint32_t m_SupportFlags = 0;
    };

}    // namespace Vega
