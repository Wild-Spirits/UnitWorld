#include "VulkanPlatform.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_WINDOWS

    #include "Utils/VulkanUtils.hpp"
    #include "Vega/Utils/Log.hpp"

    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>

    #include <vulkan/vulkan_win32.h>

namespace Vega
{

    void VulkanGetPlatformRequiredExtensionNames(WindowSystem _WindowSystem,
                                                 std::vector<const char*>* _RequiredExtensions)
    {
        (void)_WindowSystem;
        _RequiredExtensions->push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }

    bool VulkanCreateWindowSurface(VkInstance _VkInstance, const NativeWindowInfo& _WindowInfo,
                                   VkSurfaceKHR* _OutVkSurface)
    {
        *_OutVkSurface = nullptr;

        PFN_vkCreateWin32SurfaceKHR createSurface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            vkGetInstanceProcAddr(_VkInstance, "vkCreateWin32SurfaceKHR"));
        if (!createSurface)
        {
            VEGA_CORE_CRITICAL("Failed to load vkCreateWin32SurfaceKHR");
            return false;
        }

        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .hinstance = static_cast<HINSTANCE>(_WindowInfo.Display),
            .hwnd = static_cast<HWND>(_WindowInfo.Window),
        };

        VkResult result = createSurface(_VkInstance, &surfaceCreateInfo, nullptr, _OutVkSurface);
        if (!VulkanResultIsSuccess(result))
        {
            VEGA_CORE_CRITICAL("vkCreateWin32SurfaceKHR failed with result: {}", VulkanResultString(result, true));
            *_OutVkSurface = nullptr;
            return false;
        }

        return true;
    }

    bool VulkanPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                           uint32_t _QueueFamilyIndex, const NativeWindowInfo& _WindowInfo)
    {
        (void)_WindowInfo;

        PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR getPresentationSupport =
            reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(
                vkGetInstanceProcAddr(_VkInstance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
        if (!getPresentationSupport)
        {
            return false;
        }

        return getPresentationSupport(_PhysicalDevice, _QueueFamilyIndex) == VK_TRUE;
    }

}    // namespace Vega

#endif
