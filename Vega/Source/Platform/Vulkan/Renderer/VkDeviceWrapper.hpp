#pragma once

#include "VkBase.hpp"

namespace LM
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
        int32_t GraphicsFamilyIndex;
        int32_t PresentFamilyIndex;
        int32_t ComputeFamilyIndex;
        int32_t TransferFamilyIndex;
    };

    class VkDeviceWrapper
    {
    public:
        bool Init(VkInstance _VkInstance);
        void Shutdown();

    protected:
        bool SelectPhysicalDevice(VkInstance _VkInstance);

    protected:
        uint32_t m_ApiMajor;
        uint32_t m_ApiMinor;
        uint32_t m_ApiPatch;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_LogicalDevice;
        VkSwapchainSupportInfo m_SwapchainSupportInfo;

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

}    // namespace LM
