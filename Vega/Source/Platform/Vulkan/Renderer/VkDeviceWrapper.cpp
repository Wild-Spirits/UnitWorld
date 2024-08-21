#include "VkDeviceWrapper.hpp"

#include "Platform/Platform.hpp"

#ifdef LM_PLATFORM_DESKTOP
    #include "GLFW/glfw3.h"
#endif

#include <array>

namespace LM
{

    struct VkPhysicalDeviceRequirements
    {
        bool Graphics;
        bool Present;
        bool Transfer;
        bool SamplerAnisotropy;
        bool DiscreteGpu;
        bool PrioritizeDiscreteGpu;
        bool Compute;
        std::vector<std::string> DeviceExtensionNames;
    };

    struct PhysicalDeviceMeetsRequirementsResult
    {
        bool IsMeetRequirements;
        VkPhysicalDeviceQueueFamilyInfo QueueFamilyInfo;
    };

    struct BestDeviceInfo
    {
        VkPhysicalDevice PhysicalDevice = nullptr;
        VkPhysicalDeviceProperties Properties;
        VkPhysicalDeviceFeatures Features;
        VkPhysicalDeviceMemoryProperties MemoryProperties;
        VkPhysicalDeviceDriverProperties DriverProperties;
        VkPhysicalDeviceQueueFamilyInfo QueueFamilyInfo;
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT DynamicStateNext;
        VkPhysicalDeviceLineRasterizationFeaturesEXT SmoothLineNext;
        bool SupportsDeviceLocalHostVisible;
    };

    static bool VkPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                              uint32_t _QueueFamilyIndex);

    static PhysicalDeviceMeetsRequirementsResult PhysicalDeviceMeetsRequirements(
        VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice, const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features, const VkPhysicalDeviceRequirements& requirements);

    bool VkDeviceWrapper::Init(VkInstance _VkInstance)
    {
        if (!SelectPhysicalDevice(_VkInstance))
        {
            return false;
        }

        LM_CORE_INFO("Creating logical device...");
        bool presentSharesGraphisQueue =
            m_PhysicalDeviceQueueFamilyInfo.GraphicsFamilyIndex == m_PhysicalDeviceQueueFamilyInfo.PresentFamilyIndex;
        bool transferSharesGraphicsQueue =
            m_PhysicalDeviceQueueFamilyInfo.GraphicsFamilyIndex == m_PhysicalDeviceQueueFamilyInfo.TransferFamilyIndex;
        bool presentMustShareGraphicsQueue = false;

        std::vector<int32_t> indices = { m_PhysicalDeviceQueueFamilyInfo.GraphicsFamilyIndex };
        if (!presentSharesGraphisQueue)
        {
            indices.push_back(m_PhysicalDeviceQueueFamilyInfo.PresentFamilyIndex);
        }
        if (!transferSharesGraphicsQueue)
        {
            indices.push_back(m_PhysicalDeviceQueueFamilyInfo.TransferFamilyIndex);
        }

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::array<float, 2> queuePriorities = { 0.9f, 1.0f };

        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, 0);
        std::vector<VkQueueFamilyProperties> props(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, props.data());

        // TODO

        return true;
    }

    void VkDeviceWrapper::Shutdown() { }

    bool VkDeviceWrapper::SelectPhysicalDevice(VkInstance _VkInstance)
    {
        uint32_t physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(_VkInstance, &physicalDeviceCount, nullptr));
        if (physicalDeviceCount == 0)
        {
            LM_CORE_CRITICAL("No devices which support Vulkan were found");
            return false;
        }

        VkPhysicalDeviceRequirements requirements = { .Graphics = true,
                                                      .Present = true,
                                                      .Transfer = true,
                                                      .SamplerAnisotropy = true,
                                                      .DiscreteGpu = false,
                                                      .PrioritizeDiscreteGpu = true,
                                                      .DeviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME } };

#ifdef LM_PLATFORM_APPLE
        requirements.DiscreteGpu = false;
#endif

        BestDeviceInfo bestDeviceInfo = {};

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        VK_CHECK(vkEnumeratePhysicalDevices(_VkInstance, &physicalDeviceCount, physicalDevices.data()));
        for (size_t i = 0; i < physicalDevices.size(); ++i)
        {
            VkPhysicalDeviceProperties2 properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            VkPhysicalDeviceDriverProperties driverProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };
            properties2.pNext = &driverProperties;
            vkGetPhysicalDeviceProperties2(physicalDevices[i], &properties2);
            VkPhysicalDeviceProperties properties = properties2.properties;

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

            VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateNext = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT
            };
            features2.pNext = &dynamicStateNext;
            VkPhysicalDeviceLineRasterizationFeaturesEXT smoothLineNext = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT
            };
            dynamicStateNext.pNext = &smoothLineNext;
            vkGetPhysicalDeviceFeatures2(physicalDevices[i], &features2);

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memory);

            LM_CORE_INFO("Evaluating device: {}, index {}.", properties.deviceName, i);

            bool supportsDeviceLocalHostVisible = false;
            for (uint32_t j = 0; j < memory.memoryTypeCount; ++j)
            {
                if (((memory.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                    ((memory.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0))
                {
                    supportsDeviceLocalHostVisible = true;
                    break;
                }
            }

            PhysicalDeviceMeetsRequirementsResult physicalDeviceMeetsRequirementsResult =
                PhysicalDeviceMeetsRequirements(_VkInstance, physicalDevices[i], &properties, &features, requirements);

            if (physicalDeviceMeetsRequirementsResult.IsMeetRequirements)
            {
                bestDeviceInfo.PhysicalDevice = physicalDevices[i];
                bestDeviceInfo.Properties = properties;
                bestDeviceInfo.Features = features;
                bestDeviceInfo.MemoryProperties = memory;
                bestDeviceInfo.DriverProperties = driverProperties;
                bestDeviceInfo.QueueFamilyInfo = physicalDeviceMeetsRequirementsResult.QueueFamilyInfo;
                bestDeviceInfo.DynamicStateNext = dynamicStateNext;
                bestDeviceInfo.SmoothLineNext = smoothLineNext;
                bestDeviceInfo.SupportsDeviceLocalHostVisible = supportsDeviceLocalHostVisible;

                if (requirements.PrioritizeDiscreteGpu && properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    continue;
                }
                break;
            }
        }

        if (bestDeviceInfo.PhysicalDevice == nullptr)
        {
            LM_CORE_CRITICAL("No devices which meet the requirements were found.");
            return false;
        }

        LM_CORE_INFO("Selected device: {}", bestDeviceInfo.Properties.deviceName);

        switch (bestDeviceInfo.Properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER: LM_CORE_INFO("Device Type: Other"); break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: LM_CORE_INFO("Device Type: Integrated GPU"); break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: LM_CORE_INFO("Device Type: Discrete GPU"); break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: LM_CORE_INFO("Device Type: Virtual GPU"); break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: LM_CORE_INFO("Device Type: CPU"); break;
            default: LM_CORE_INFO("Device Type: Unknown"); break;
        }

        LM_CORE_INFO("Device Driver version : {}", bestDeviceInfo.DriverProperties.driverInfo);

        m_ApiMajor = VK_VERSION_MAJOR(bestDeviceInfo.Properties.apiVersion);
        m_ApiMinor = VK_VERSION_MINOR(bestDeviceInfo.Properties.apiVersion);
        m_ApiPatch = VK_VERSION_PATCH(bestDeviceInfo.Properties.apiVersion);

        LM_CORE_INFO("Vulkan API Version: {}.{}.{}", m_ApiMajor, m_ApiMinor, m_ApiPatch);

        for (uint32_t i = 0; i < bestDeviceInfo.MemoryProperties.memoryHeapCount; ++i)
        {
            float memorySizeGiB =
                static_cast<float>(bestDeviceInfo.MemoryProperties.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f;
            if (bestDeviceInfo.MemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                LM_CORE_INFO("Device Local Memory: {:.2f} GiB", memorySizeGiB);
            }
            else
            {
                LM_CORE_INFO("Shared System Memory: {:.2f} GiB", memorySizeGiB);
            }
        }

        m_PhysicalDevice = bestDeviceInfo.PhysicalDevice;
        m_PhysicalDeviceQueueFamilyInfo = bestDeviceInfo.QueueFamilyInfo;
        m_PhysicalDeviceProperties = bestDeviceInfo.Properties;
        m_PhysicalDeviceFeatures = bestDeviceInfo.Features;
        m_PhysicalDeviceMemoryProperties = bestDeviceInfo.MemoryProperties;
        m_SupportDeviceLocalHostVisible = bestDeviceInfo.SupportsDeviceLocalHostVisible;

        if (m_ApiMajor >= 1 && m_ApiMinor >= 2)
        {
            m_SupportFlags |= VkDeviceSupportFlagBits::kNativeDynamicStateBit;
        }
        if (bestDeviceInfo.DynamicStateNext.extendedDynamicState)
        {

            m_SupportFlags |= VkDeviceSupportFlagBits::kDynamicStateBit;
        }
        if (bestDeviceInfo.SmoothLineNext.smoothLines)
        {
            m_SupportFlags |= VkDeviceSupportFlagBits::kLineSmoothRasterisationBit;
        }

        return true;
    }

    PhysicalDeviceMeetsRequirementsResult PhysicalDeviceMeetsRequirements(
        VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice, const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features, const VkPhysicalDeviceRequirements& requirements)
    {
        if (requirements.DiscreteGpu)
        {
            if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                LM_CORE_INFO("Device is not a discrete GPU, and one is required. Skipping.");
                return { false };
            }
        }

        VkPhysicalDeviceQueueFamilyInfo outQueueInfo = {
            .GraphicsFamilyIndex = -1, .PresentFamilyIndex = -1, .ComputeFamilyIndex = -1, .TransferFamilyIndex = -1
        };

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

        LM_CORE_INFO("Graphics | Present | Compute | Transfer | Name");
        uint8_t minTransferScore = 255;
        for (size_t i = 0; i < queueFamilies.size(); ++i)
        {
            uint8_t currentTransferScore = 0;

            if (outQueueInfo.GraphicsFamilyIndex == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                outQueueInfo.GraphicsFamilyIndex = i;
                ++currentTransferScore;

                bool supportsPresent = VkPlatformPresentationSupport(_VkInstance, _PhysicalDevice, i);
                if (supportsPresent)
                {
                    outQueueInfo.PresentFamilyIndex = i;
                    ++currentTransferScore;
                }
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                outQueueInfo.ComputeFamilyIndex = i;
                ++currentTransferScore;
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                if (currentTransferScore <= minTransferScore)
                {
                    minTransferScore = currentTransferScore;
                    outQueueInfo.TransferFamilyIndex = i;
                }
            }
        }

        // Print out some info about the device
        LM_CORE_INFO("  {:5}  |  {:5}  |  {:5}  |  {:5}   | {}", outQueueInfo.GraphicsFamilyIndex != -1,
                     outQueueInfo.PresentFamilyIndex != -1, outQueueInfo.ComputeFamilyIndex != -1,
                     outQueueInfo.TransferFamilyIndex != -1, properties->deviceName);

        if (outQueueInfo.PresentFamilyIndex == -1)
        {
            for (size_t i = 0; i < queueFamilies.size(); ++i)
            {
                bool supportsPresent = VkPlatformPresentationSupport(_VkInstance, _PhysicalDevice, i);
                if (supportsPresent)
                {
                    outQueueInfo.PresentFamilyIndex = i;

                    if (outQueueInfo.PresentFamilyIndex != outQueueInfo.GraphicsFamilyIndex)
                    {
                        LM_CORE_WARN("Warning: Different queue index used for present vs graphics: {}.", i);
                    }
                    break;
                }
            }
        }

        if ((!requirements.Graphics || (requirements.Graphics && outQueueInfo.GraphicsFamilyIndex != -1)) &&
            (!requirements.Present || (requirements.Present && outQueueInfo.PresentFamilyIndex != -1)) &&
            (!requirements.Compute || (requirements.Compute && outQueueInfo.ComputeFamilyIndex != -1)) &&
            (!requirements.Transfer || (requirements.Transfer && outQueueInfo.TransferFamilyIndex != -1)))
        {
            LM_CORE_INFO("Device meets queue requirements.");
            LM_CORE_TRACE("Graphics Family Index: {}", outQueueInfo.GraphicsFamilyIndex);
            LM_CORE_TRACE("Present Family Index:  {}", outQueueInfo.PresentFamilyIndex);
            LM_CORE_TRACE("Transfer Family Index: {}", outQueueInfo.TransferFamilyIndex);
            LM_CORE_TRACE("Compute Family Index:  {}", outQueueInfo.ComputeFamilyIndex);

            if (requirements.DeviceExtensionNames.size() > 0)
            {
                uint32_t availableExtensionCount = 0;
                VK_CHECK(vkEnumerateDeviceExtensionProperties(_PhysicalDevice, 0, &availableExtensionCount, nullptr));
                if (availableExtensionCount != 0)
                {
                    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
                    VK_CHECK(vkEnumerateDeviceExtensionProperties(_PhysicalDevice, 0, &availableExtensionCount,
                                                                  availableExtensions.data()));

                    for (size_t i = 0; i < requirements.DeviceExtensionNames.size(); ++i)
                    {
                        bool found = false;
                        for (size_t j = 0; j < availableExtensions.size(); ++j)
                        {
                            if (requirements.DeviceExtensionNames[i] == availableExtensions[j].extensionName)
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            LM_CORE_INFO("Required extension not found: {}, skipping device.",
                                         requirements.DeviceExtensionNames[i]);
                            return { false };
                        }
                    }
                }
            }

            // Sampler anisotropy
            if (requirements.SamplerAnisotropy && !features->samplerAnisotropy)
            {
                LM_CORE_INFO("Device does not support samplerAnisotropy, skipping.");
                return { false };
            }

            // Device meets all requirements.
            return { true, outQueueInfo };
        }

        return { false };
    }

    bool VkPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                       uint32_t _QueueFamilyIndex)
    {
#ifdef LM_PLATFORM_DESKTOP
        return glfwGetPhysicalDevicePresentationSupport(_VkInstance, _PhysicalDevice, _QueueFamilyIndex);
#endif
        return false;
    }

}    // namespace LM
