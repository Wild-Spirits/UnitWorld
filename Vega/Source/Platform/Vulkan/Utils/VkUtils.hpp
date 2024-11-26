#pragma once

#include <string>

#include <vulkan/vulkan.h>

namespace Vega
{

    bool VkResultIsSuccess(VkResult _Result);

    std::string VkResultString(VkResult _Result, bool _GetExtended);

#if defined(_DEBUG)

    void VkSetDebugObjectName(PFN_vkSetDebugUtilsObjectNameEXT _PfnSetDebugUtilsObjectNameEXT, VkDevice _LogicalDevice,
                              VkObjectType _ObjectType, void* _ObjectHandle, const char* _ObjectName);

    #define VK_SET_DEBUG_OBJECT_NAME(_PfnSetDebugUtilsObjectNameEXT, _LogicalDevice, _ObjectType, _ObjectHandle,       \
                                     _ObjectName)                                                                      \
        VkSetDebugObjectName(_PfnSetDebugUtilsObjectNameEXT, _LogicalDevice, _ObjectType, _ObjectHandle, _ObjectName)

#else
    #define VK_SET_DEBUG_OBJECT_NAME(_PfnSetDebugUtilsObjectNameEXT, _LogicalDevice, _ObjectType, _ObjectHandle,       \
                                     _ObjectName)
#endif

}    // namespace Vega
