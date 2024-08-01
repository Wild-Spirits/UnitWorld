#include "VkPlatform.hpp"

#include "Platform/Platform.hpp"

#ifdef LM_PLATFORM_WINDOWS

    #include <Windows.h>
    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_win32.h>

namespace LM
{

    void VkGetPlatformRequiredExtensionNames(std::vector<const char*>* _RequiredExtensions)
    {
        _RequiredExtensions->push_back("VK_KHR_win32_surface");
    }

}    // namespace LM

#endif
