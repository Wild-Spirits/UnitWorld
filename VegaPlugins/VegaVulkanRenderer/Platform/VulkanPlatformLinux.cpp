#include "VulkanPlatform.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_LINUX

    #include "GLFW/glfw3.h"

    #include <cstring>

    #include <vulkan/vulkan.h>

namespace Vega
{

    void VulkanGetPlatformRequiredExtensionNames(std::vector<const char*>* _RequiredExtensions)
    {
        uint32_t count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        if (extensions == nullptr)
        {
            return;
        }

        // VK_KHR_SURFACE_EXTENSION_NAME is already in the list by the caller; GLFW returns it too.
        for (uint32_t i = 0; i < count; ++i)
        {
            const char* extension = extensions[i];
            if (std::strcmp(extension, VK_KHR_SURFACE_EXTENSION_NAME) != 0)
            {
                _RequiredExtensions->push_back(extension);
            }
        }
    }

}    // namespace Vega

#endif
