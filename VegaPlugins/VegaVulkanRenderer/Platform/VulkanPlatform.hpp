#pragma once

#include "Vega/Core/Window.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace Vega
{

    void VulkanGetPlatformRequiredExtensionNames(WindowSystem _WindowSystem,
                                                 std::vector<const char*>* _RequiredExtensions);

    // Creates a window system surface from the engine's native window handles (NativeWindowInfo): they belong
    // to the connection that owns the window, while a GLFW copy inside this plugin would open its own one
    bool VulkanCreateWindowSurface(VkInstance _VkInstance, const NativeWindowInfo& _WindowInfo,
                                   VkSurfaceKHR* _OutVkSurface);

    bool VulkanPlatformPresentationSupport(VkInstance _VkInstance, VkPhysicalDevice _PhysicalDevice,
                                           uint32_t _QueueFamilyIndex, const NativeWindowInfo& _WindowInfo);

}    // namespace Vega
