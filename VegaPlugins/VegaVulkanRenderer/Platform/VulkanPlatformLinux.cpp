#include "VulkanPlatform.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_LINUX

    #include "Utils/VulkanUtils.hpp"
    #include "Vega/Utils/Log.hpp"

    // Window system headers must precede the WSI headers: vulkan_xlib.h uses Display/Window without
    // including Xlib.h itself
    #include <wayland-client.h>
    #include <X11/Xlib.h>

    #include <vulkan/vulkan_wayland.h>
    #include <vulkan/vulkan_xlib.h>

namespace Vega
{

    void VulkanGetPlatformRequiredExtensionNames(WindowSystem _WindowSystem,
                                                 std::vector<const char*>* _RequiredExtensions)
    {
        switch (_WindowSystem)
        {
            case WindowSystem::kWayland:
                _RequiredExtensions->push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
                break;
            case WindowSystem::kX11:
                _RequiredExtensions->push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
                break;
            default:
                VEGA_CORE_CRITICAL("Unsupported window system for the Vulkan backend: {}",
                                   static_cast<int32_t>(_WindowSystem));
                break;
        }
    }

    bool VulkanCreateWindowSurface(VkInstance _VkInstance, const NativeWindowInfo& _WindowInfo,
                                   VkSurfaceKHR* _OutVkSurface)
    {
        *_OutVkSurface = nullptr;

        VkResult result = VK_ERROR_INITIALIZATION_FAILED;
        switch (_WindowInfo.System)
        {
            case WindowSystem::kWayland:
            {
                PFN_vkCreateWaylandSurfaceKHR createSurface = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
                    vkGetInstanceProcAddr(_VkInstance, "vkCreateWaylandSurfaceKHR"));
                if (!createSurface)
                {
                    VEGA_CORE_CRITICAL("Failed to load vkCreateWaylandSurfaceKHR");
                    return false;
                }

                VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
                    .pNext = nullptr,
                    .flags = 0,
                    .display = static_cast<wl_display*>(_WindowInfo.Display),
                    .surface = static_cast<wl_surface*>(_WindowInfo.Window),
                };
                result = createSurface(_VkInstance, &surfaceCreateInfo, nullptr, _OutVkSurface);
                break;
            }
            case WindowSystem::kX11:
            {
                PFN_vkCreateXlibSurfaceKHR createSurface = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
                    vkGetInstanceProcAddr(_VkInstance, "vkCreateXlibSurfaceKHR"));
                if (!createSurface)
                {
                    VEGA_CORE_CRITICAL("Failed to load vkCreateXlibSurfaceKHR");
                    return false;
                }

                VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                    .pNext = nullptr,
                    .flags = 0,
                    .dpy = static_cast<Display*>(_WindowInfo.Display),
                    // `::Window`: inside namespace Vega the unqualified name is the engine's Window class
                    .window = static_cast<::Window>(reinterpret_cast<uintptr_t>(_WindowInfo.Window)),
                };
                result = createSurface(_VkInstance, &surfaceCreateInfo, nullptr, _OutVkSurface);
                break;
            }
            default:
                VEGA_CORE_CRITICAL("Unsupported window system for the Vulkan backend: {}",
                                   static_cast<int32_t>(_WindowInfo.System));
                return false;
        }

        if (!VulkanResultIsSuccess(result))
        {
            VEGA_CORE_CRITICAL("Failed to create platform surface with result: {}", VulkanResultString(result, true));
            *_OutVkSurface = nullptr;
            return false;
        }

        return true;
    }

    bool VulkanPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                           uint32_t _QueueFamilyIndex, const NativeWindowInfo& _WindowInfo)
    {
        switch (_WindowInfo.System)
        {
            case WindowSystem::kWayland:
            {
                PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR getPresentationSupport =
                    reinterpret_cast<PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR>(
                        vkGetInstanceProcAddr(_VkInstance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR"));
                if (!getPresentationSupport)
                {
                    return false;
                }

                return getPresentationSupport(_PhysicalDevice, _QueueFamilyIndex,
                                              static_cast<wl_display*>(_WindowInfo.Display)) == VK_TRUE;
            }
            case WindowSystem::kX11:
            {
                PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR getPresentationSupport =
                    reinterpret_cast<PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR>(
                        vkGetInstanceProcAddr(_VkInstance, "vkGetPhysicalDeviceXlibPresentationSupportKHR"));
                if (!getPresentationSupport)
                {
                    return false;
                }

                Display* display = static_cast<Display*>(_WindowInfo.Display);
                ::Window window = static_cast<::Window>(reinterpret_cast<uintptr_t>(_WindowInfo.Window));

                // The WSI call expects the visual of the window; it is only known to the X server
                XWindowAttributes windowAttributes;
                if (XGetWindowAttributes(display, window, &windowAttributes) == 0)
                {
                    return false;
                }

                return getPresentationSupport(_PhysicalDevice, _QueueFamilyIndex, display,
                                              XVisualIDFromVisual(windowAttributes.visual)) == VK_TRUE;
            }
            default: return false;
        }
    }

}    // namespace Vega

#endif
