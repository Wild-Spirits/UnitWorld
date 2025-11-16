#include "VulkanDeviceWrapper.hpp"

#include "Platform/Platform.hpp"
#include "Utils/VulkanUtils.hpp"

#ifdef VEGA_PLATFORM_DESKTOP
    #include "GLFW/glfw3.h"
#endif

#include <array>

// TODO: use Aggregate with designated initializers for VK structs

namespace Vega
{

    struct VulkanPhysicalDeviceRequirements
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
        VulkanPhysicalDeviceQueueFamilyInfo QueueFamilyInfo;
    };

    struct BestDeviceInfo
    {
        VkPhysicalDevice PhysicalDevice = nullptr;
        VkPhysicalDeviceProperties Properties;
        VkPhysicalDeviceFeatures Features;
        VkPhysicalDeviceMemoryProperties MemoryProperties;
        VkPhysicalDeviceDriverProperties DriverProperties;
        VulkanPhysicalDeviceQueueFamilyInfo QueueFamilyInfo;
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT DynamicStateNext;
        VkPhysicalDeviceLineRasterizationFeaturesEXT SmoothLineNext;
        bool SupportsDeviceLocalHostVisible;
    };

    static bool VulkanPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                                  uint32_t _QueueQueueIndex);

    static PhysicalDeviceMeetsRequirementsResult PhysicalDeviceMeetsRequirements(
        VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice, const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features, const VulkanPhysicalDeviceRequirements& requirements);

    bool VulkanDeviceWrapper::Init(VulkanContext& _Context)
    {
        using namespace std::literals;

        if (!SelectPhysicalDevice(_Context.VkInstance))
        {
            return false;
        }

        VEGA_CORE_INFO("Creating logical device...");
        bool presentSharesGraphicsQueue =
            m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex == m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex;
        bool transferSharesGraphicsQueue =
            m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex == m_PhysicalDeviceQueueFamilyInfo.TransferQueueIndex;
        bool presentMustShareGraphicsQueue = false;

        std::vector<int32_t> indices = { m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex };
        if (!presentSharesGraphicsQueue)
        {
            indices.push_back(m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex);
        }
        if (!transferSharesGraphicsQueue)
        {
            indices.push_back(m_PhysicalDeviceQueueFamilyInfo.TransferQueueIndex);
        }

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(indices.size());
        std::array<float, 2> queuePriorities = { 0.9f, 1.0f };

        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, nullptr);
        std::vector<VkQueueFamilyProperties> props(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, props.data());

        for (uint32_t i = 0; i < static_cast<uint32_t>(indices.size()); ++i)
        {
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = indices[i];
            queueCreateInfos[i].queueCount = 1;

            if (presentSharesGraphicsQueue && indices[i] == m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex)
            {
                if (props[m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex].queueCount > 1)
                {
                    queueCreateInfos[i].queueCount = 2;
                }
                else
                {
                    presentMustShareGraphicsQueue = true;
                }
            }

            // TODO: Enable this for a future enhancement.
            // if (indices[i] == m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex) {
            //     queueCreateInfos[i].queueCount = 2;
            // }
            queueCreateInfos[i].flags = 0;
            queueCreateInfos[i].pNext = nullptr;
            queueCreateInfos[i].pQueuePriorities = queuePriorities.data();
        }

        bool portabilityRequired = false;
        uint32_t availableExtensionCount = 0;
        VkExtensionProperties* availableExtensions = 0;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &availableExtensionCount, nullptr));
        if (availableExtensionCount != 0)
        {
            std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &availableExtensionCount,
                                                          availableExtensions.data()));
            for (uint32_t i = 0; i < availableExtensionCount; ++i)
            {
                if (std::string_view(availableExtensions[i].extensionName) == "VK_KHR_portability_subset"sv)
                {
                    VEGA_CORE_INFO("Adding required extension 'VK_KHR_portability_subset'.");
                    portabilityRequired = true;
                    break;
                }
            }
        }

        std::vector<const char*> extensionNames;
        extensionNames.reserve(8);
        extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        extensionNames.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

        if (portabilityRequired)
        {
            extensionNames.push_back("VK_KHR_portability_subset");
        }

        if (((m_SupportFlags & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit) == 0) &&
            ((m_SupportFlags & VulkanDeviceSupportFlagBits::kDynamicStateBit) != 0))
        {
            extensionNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        }
        extensionNames.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

        if ((m_SupportFlags & VulkanDeviceSupportFlagBits::kLineSmoothRasterizationBit))
        {
            extensionNames.push_back(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME);
        }

        VkPhysicalDeviceFeatures2 deviceFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
        {    // TODO: remove scope???
            deviceFeatures.features.samplerAnisotropy = m_PhysicalDeviceFeatures.samplerAnisotropy;
            deviceFeatures.features.fillModeNonSolid = m_PhysicalDeviceFeatures.fillModeNonSolid;

            VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
                .descriptorBindingPartiallyBound = VK_TRUE,    // TODO: Check if supported?
            };
            VkPhysicalDeviceSynchronization2Features sync2Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                .pNext = &descriptorIndexingFeatures,
                .synchronization2 = VK_TRUE,
            };
            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
                .pNext = &sync2Features,
                .extendedDynamicState = VK_TRUE,
            };
            VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingExt = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
                .pNext = &extendedDynamicStateFeatures,
                .dynamicRendering = VK_TRUE,
            };
            deviceFeatures.pNext = &dynamicRenderingExt;

#if defined(VK_USE_PLATFORM_MACOS_MVK)
            setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
#endif

            if (m_SupportFlags & VulkanDeviceSupportFlagBits::kLineSmoothRasterizationBit)
            {
                VkPhysicalDeviceLineRasterizationFeaturesEXT lineRasterizationExt = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT,
                    .smoothLines = VK_TRUE,
                };
                descriptorIndexingFeatures.pNext = &lineRasterizationExt;
            }
        }

        VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &deviceFeatures,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
            .ppEnabledExtensionNames = extensionNames.data(),
            .pEnabledFeatures = nullptr,
        };

        VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, _Context.VkAllocator, &m_LogicalDevice));

        VK_SET_DEBUG_OBJECT_NAME(_Context.PfnSetDebugUtilsObjectNameEXT, m_LogicalDevice, VK_OBJECT_TYPE_DEVICE,
                                 m_LogicalDevice, "Vulkan Logical Device");

        VEGA_CORE_INFO("Logical device created.");

        if (!(m_SupportFlags & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit) &&
            (m_SupportFlags & VulkanDeviceSupportFlagBits::kDynamicStateBit))
        {
            VEGA_CORE_INFO(
                "Vulkan device doesn't support native dynamic state, but does via extension. Using extension.");

            _Context.VkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT)vkGetInstanceProcAddr(
                _Context.VkInstance, "vkCmdSetPrimitiveTopologyEXT");

            _Context.VkCmdSetFrontFaceEXT =
                (PFN_vkCmdSetFrontFaceEXT)vkGetInstanceProcAddr(_Context.VkInstance, "vkCmdSetFrontFaceEXT");

            _Context.VkCmdSetStencilOpEXT =
                (PFN_vkCmdSetStencilOpEXT)vkGetInstanceProcAddr(_Context.VkInstance, "vkCmdSetStencilOpEXT");
            _Context.VkCmdSetStencilTestEnableEXT = (PFN_vkCmdSetStencilTestEnableEXT)vkGetInstanceProcAddr(
                _Context.VkInstance, "vkCmdSetStencilTestEnableEXT");
            _Context.VkCmdSetDepthTestEnableEXT = (PFN_vkCmdSetDepthTestEnableEXT)vkGetInstanceProcAddr(
                _Context.VkInstance, "vkCmdSetDepthTestEnableEXT");
            _Context.VkCmdSetDepthWriteEnableEXT = (PFN_vkCmdSetDepthWriteEnableEXT)vkGetInstanceProcAddr(
                _Context.VkInstance, "vkCmdSetDepthWriteEnableEXT");

            _Context.VkCmdBeginRenderingKHR =
                (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(_Context.VkInstance, "vkCmdBeginRenderingKHR");
            _Context.VkCmdEndRenderingKHR =
                (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(_Context.VkInstance, "vkCmdEndRenderingKHR");
        }
        else
        {
            if (m_SupportFlags & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
            {
                VEGA_CORE_INFO("Vulkan device supports native dynamic state and dynamic rendering.");
            }
            else
            {
                VEGA_CORE_WARN(
                    "Vulkan device does not support native or extension dynamic state. This may cause issues with "
                    "the renderer.");
            }
        }

        vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex, 0, &m_GraphicsQueue);

        vkGetDeviceQueue(
            m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex,
            // If the same family is shared between graphic and presentation,
            // pull from the second index instead of the first for a unique queue.
            presentMustShareGraphicsQueue ? 0
            : (m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex == m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex)
                ? 1
                : 0,
            &m_PresentQueue);

        vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.TransferQueueIndex, 0, &m_TransferQueue);
        VEGA_CORE_INFO("Queues obtained.");

        VkCommandPoolCreateInfo poolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = static_cast<uint32_t>(m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex),
        };
        VK_CHECK(vkCreateCommandPool(m_LogicalDevice, &poolCreateInfo, _Context.VkAllocator, &m_GraphicsCommandPool));
        VEGA_CORE_INFO("Graphics command pool created.");

        DetectDepthFormat();

        return true;
    }

    void VulkanDeviceWrapper::Shutdown(const VulkanContext& _Context)
    {
        m_GraphicsQueue = nullptr;
        m_PresentQueue = nullptr;
        m_TransferQueue = nullptr;
        m_ComputeQueue = nullptr;

        VEGA_CORE_INFO("Destroying command pools...");
        vkDestroyCommandPool(m_LogicalDevice, m_GraphicsCommandPool, _Context.VkAllocator);

        VEGA_CORE_INFO("Destroying logical device...");
        vkDestroyDevice(m_LogicalDevice, _Context.VkAllocator);
        m_LogicalDevice = nullptr;

        VEGA_CORE_INFO("Releasing physical device resources...");
        m_PhysicalDevice = nullptr;

        m_PhysicalDeviceQueueFamilyInfo = {
            .GraphicsQueueIndex = -1,
            .PresentQueueIndex = -1,
            .ComputeQueueIndex = -1,
            .TransferQueueIndex = -1,
        };
    }

    VulkanSwapchainSupportInfo VulkanDeviceWrapper::GetSwapchainSupportInfo(VkSurfaceKHR _Surface) const
    {
        VulkanSwapchainSupportInfo swapchainSupportInfo = { 0 };

        VkResult capabilitiesResult =
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, _Surface, &swapchainSupportInfo.Capabilities);

        if (!VulkanResultIsSuccess(capabilitiesResult))
        {
            VEGA_CORE_CRITICAL("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed with message: {}",
                               (capabilitiesResult, true));
            VEGA_CORE_ASSERT(false, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed!");
        }

        uint32_t formatCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, _Surface, &formatCount, 0));
        if (formatCount != 0)
        {
            swapchainSupportInfo.Formats.resize(formatCount);
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, _Surface, &formatCount,
                                                          swapchainSupportInfo.Formats.data()));
        }

        uint32_t presentModeCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, _Surface, &presentModeCount, 0));
        if (presentModeCount != 0)
        {
            swapchainSupportInfo.PresentModes.resize(presentModeCount);
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, _Surface, &presentModeCount,
                                                               swapchainSupportInfo.PresentModes.data()));
        }

        return swapchainSupportInfo;
    }

    void VulkanDeviceWrapper::DetectDepthFormat()
    {
        std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        std::vector<uint8_t> sizes = { 4, 3 };

        uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, candidates[i], &properties);

            if ((properties.linearTilingFeatures & flags) == flags)
            {
                m_DepthFormat = candidates[i];
                m_DepthChannelCount = sizes[i];
                return;
            }
            else if ((properties.optimalTilingFeatures & flags) == flags)
            {
                m_DepthFormat = candidates[i];
                m_DepthChannelCount = sizes[i];
                return;
            }
        }

        VEGA_CORE_ASSERT(false, "Failed to find a suitable depth format.");
    }

    uint32_t VulkanDeviceWrapper::GetMemoryTypeIndex(uint32_t _TypeBits, VkMemoryPropertyFlags _Properties) const
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if ((_TypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & _Properties) == _Properties)
            {
                return i;
            }
        }

        VEGA_CORE_ASSERT(false, "Failed to find memory type index.");

        return 0;
    }

    bool VulkanDeviceWrapper::SelectPhysicalDevice(VkInstance _VkInstance)
    {
        uint32_t physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(_VkInstance, &physicalDeviceCount, nullptr));
        if (physicalDeviceCount == 0)
        {
            VEGA_CORE_CRITICAL("No devices which support Vulkan were found");
            return false;
        }

        VulkanPhysicalDeviceRequirements requirements = {
            .Graphics = true,
            .Present = true,
            .Transfer = true,
            .SamplerAnisotropy = true,
            .DiscreteGpu = false,
            .PrioritizeDiscreteGpu = true,
            .DeviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
        };

#ifdef VEGA_PLATFORM_APPLE
        requirements.DiscreteGpu = false;
#endif

        BestDeviceInfo bestDeviceInfo = {};

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        VK_CHECK(vkEnumeratePhysicalDevices(_VkInstance, &physicalDeviceCount, physicalDevices.data()));
        for (size_t i = 0; i < physicalDevices.size(); ++i)
        {
            VkPhysicalDeviceDriverProperties driverProperties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
            };
            VkPhysicalDeviceProperties2 properties2 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &driverProperties,
            };
            vkGetPhysicalDeviceProperties2(physicalDevices[i], &properties2);
            VkPhysicalDeviceProperties properties = properties2.properties;

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

            VkPhysicalDeviceLineRasterizationFeaturesEXT smoothLineNext = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT,
            };
            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateNext = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
                .pNext = &smoothLineNext,
            };
            VkPhysicalDeviceFeatures2 features2 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &dynamicStateNext,
            };
            vkGetPhysicalDeviceFeatures2(physicalDevices[i], &features2);

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memory);

            VEGA_CORE_INFO("Evaluating device: {}, index {}.", properties.deviceName, i);

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
            VEGA_CORE_CRITICAL("No devices which meet the requirements were found.");
            return false;
        }

        VEGA_CORE_INFO("Selected device: {}", bestDeviceInfo.Properties.deviceName);

        switch (bestDeviceInfo.Properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER: VEGA_CORE_INFO("Device Type: Other"); break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: VEGA_CORE_INFO("Device Type: Integrated GPU"); break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: VEGA_CORE_INFO("Device Type: Discrete GPU"); break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: VEGA_CORE_INFO("Device Type: Virtual GPU"); break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: VEGA_CORE_INFO("Device Type: CPU"); break;
            default: VEGA_CORE_INFO("Device Type: Unknown"); break;
        }

        VEGA_CORE_INFO("Device Driver version : {}", bestDeviceInfo.DriverProperties.driverInfo);

        m_ApiMajor = VK_VERSION_MAJOR(bestDeviceInfo.Properties.apiVersion);
        m_ApiMinor = VK_VERSION_MINOR(bestDeviceInfo.Properties.apiVersion);
        m_ApiPatch = VK_VERSION_PATCH(bestDeviceInfo.Properties.apiVersion);

        VEGA_CORE_INFO("Vulkan API Version: {}.{}.{}", m_ApiMajor, m_ApiMinor, m_ApiPatch);

        for (uint32_t i = 0; i < bestDeviceInfo.MemoryProperties.memoryHeapCount; ++i)
        {
            float memorySizeGiB =
                static_cast<float>(bestDeviceInfo.MemoryProperties.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f;
            if (bestDeviceInfo.MemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                VEGA_CORE_INFO("Device Local Memory: {:.2f} GiB", memorySizeGiB);
            }
            else
            {
                VEGA_CORE_INFO("Shared System Memory: {:.2f} GiB", memorySizeGiB);
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
            m_SupportFlags |= VulkanDeviceSupportFlagBits::kNativeDynamicStateBit;
        }
        if (bestDeviceInfo.DynamicStateNext.extendedDynamicState)
        {

            m_SupportFlags |= VulkanDeviceSupportFlagBits::kDynamicStateBit;
        }
        if (bestDeviceInfo.SmoothLineNext.smoothLines)
        {
            m_SupportFlags |= VulkanDeviceSupportFlagBits::kLineSmoothRasterizationBit;
        }

        return true;
    }

    PhysicalDeviceMeetsRequirementsResult PhysicalDeviceMeetsRequirements(
        VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice, const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features, const VulkanPhysicalDeviceRequirements& requirements)
    {
        if (requirements.DiscreteGpu)
        {
            if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                VEGA_CORE_INFO("Device is not a discrete GPU, and one is required. Skipping.");
                return { false };
            }
        }

        VulkanPhysicalDeviceQueueFamilyInfo outQueueInfo = {
            .GraphicsQueueIndex = -1,
            .PresentQueueIndex = -1,
            .ComputeQueueIndex = -1,
            .TransferQueueIndex = -1,
        };

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

        VEGA_CORE_INFO("Graphics | Present | Compute | Transfer | Name");
        uint8_t minTransferScore = 255;
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            uint8_t currentTransferScore = 0;

            if (outQueueInfo.GraphicsQueueIndex == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                outQueueInfo.GraphicsQueueIndex = i;
                ++currentTransferScore;

                bool supportsPresent = VulkanPlatformPresentationSupport(_VkInstance, _PhysicalDevice, i);
                if (supportsPresent)
                {
                    outQueueInfo.PresentQueueIndex = i;
                    ++currentTransferScore;
                }
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                outQueueInfo.ComputeQueueIndex = i;
                ++currentTransferScore;
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                if (currentTransferScore <= minTransferScore)
                {
                    minTransferScore = currentTransferScore;
                    outQueueInfo.TransferQueueIndex = i;
                }
            }
        }

        // Print out some info about the device
        VEGA_CORE_INFO("  {:5}  |  {:5}  |  {:5}  |  {:5}   | {}", outQueueInfo.GraphicsQueueIndex != -1,
                       outQueueInfo.PresentQueueIndex != -1, outQueueInfo.ComputeQueueIndex != -1,
                       outQueueInfo.TransferQueueIndex != -1, properties->deviceName);

        if (outQueueInfo.PresentQueueIndex == -1)
        {
            for (uint32_t i = 0; i < queueFamilyCount; ++i)
            {
                bool supportsPresent = VulkanPlatformPresentationSupport(_VkInstance, _PhysicalDevice, i);
                if (supportsPresent)
                {
                    outQueueInfo.PresentQueueIndex = i;

                    if (outQueueInfo.PresentQueueIndex != outQueueInfo.GraphicsQueueIndex)
                    {
                        VEGA_CORE_WARN("Warning: Different queue index used for present vs graphics: {}.", i);
                    }
                    break;
                }
            }
        }

        if ((!requirements.Graphics || (requirements.Graphics && outQueueInfo.GraphicsQueueIndex != -1)) &&
            (!requirements.Present || (requirements.Present && outQueueInfo.PresentQueueIndex != -1)) &&
            (!requirements.Compute || (requirements.Compute && outQueueInfo.ComputeQueueIndex != -1)) &&
            (!requirements.Transfer || (requirements.Transfer && outQueueInfo.TransferQueueIndex != -1)))
        {
            VEGA_CORE_INFO("Device meets queue requirements.");
            VEGA_CORE_TRACE("Graphics Family Index: {}", outQueueInfo.GraphicsQueueIndex);
            VEGA_CORE_TRACE("Present Family Index:  {}", outQueueInfo.PresentQueueIndex);
            VEGA_CORE_TRACE("Transfer Family Index: {}", outQueueInfo.TransferQueueIndex);
            VEGA_CORE_TRACE("Compute Family Index:  {}", outQueueInfo.ComputeQueueIndex);

            if (requirements.DeviceExtensionNames.size() > 0)
            {
                uint32_t availableExtensionCount = 0;
                VK_CHECK(
                    vkEnumerateDeviceExtensionProperties(_PhysicalDevice, nullptr, &availableExtensionCount, nullptr));
                if (availableExtensionCount != 0)
                {
                    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
                    VK_CHECK(vkEnumerateDeviceExtensionProperties(_PhysicalDevice, nullptr, &availableExtensionCount,
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
                            VEGA_CORE_INFO("Required extension not found: {}, skipping device.",
                                           requirements.DeviceExtensionNames[i]);
                            return { false };
                        }
                    }
                }
            }

            // Sampler anisotropy
            if (requirements.SamplerAnisotropy && !features->samplerAnisotropy)
            {
                VEGA_CORE_INFO("Device does not support samplerAnisotropy, skipping.");
                return { false };
            }

            // Device meets all requirements.
            return { true, outQueueInfo };
        }

        return { false };
    }

    bool VulkanPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                           uint32_t _QueueQueueIndex)
    {
#ifdef VEGA_PLATFORM_DESKTOP
        return glfwGetPhysicalDevicePresentationSupport(_VkInstance, _PhysicalDevice, _QueueQueueIndex);
#endif
        return false;
    }

}    // namespace Vega
