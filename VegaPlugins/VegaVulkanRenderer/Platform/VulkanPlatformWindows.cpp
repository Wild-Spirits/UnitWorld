#include "VulkanPlatform.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_WINDOWS

    #include "GLFW/glfw3.h"

    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>

    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_win32.h>

namespace Vega
{

    void VulkanGetPlatformRequiredExtensionNames(std::vector<const char*>* _RequiredExtensions)
    {
        _RequiredExtensions->push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }

}    // namespace Vega

#endif
