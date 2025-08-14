#pragma once

#include <string>

#include <vulkan/vulkan.h>

namespace Vega
{

    bool VulkanResultIsSuccess(VkResult _Result);

    std::string VulkanResultString(VkResult _Result, bool _GetExtended);

#ifdef _DEBUG

    void VulkanSetDebugObjectName(PFN_vkSetDebugUtilsObjectNameEXT _PfnSetDebugUtilsObjectNameEXT,
                                  VkDevice _LogicalDevice, VkObjectType _ObjectType, void* _ObjectHandle,
                                  const char* _ObjectName);

    #define VK_SET_DEBUG_OBJECT_NAME(_PfnSetDebugUtilsObjectNameEXT, _LogicalDevice, _ObjectType, _ObjectHandle,       \
                                     _ObjectName)                                                                      \
        VulkanSetDebugObjectName(_PfnSetDebugUtilsObjectNameEXT, _LogicalDevice, _ObjectType, _ObjectHandle,           \
                                 _ObjectName)

#else
    #define VK_SET_DEBUG_OBJECT_NAME(_PfnSetDebugUtilsObjectNameEXT, _LogicalDevice, _ObjectType, _ObjectHandle,       \
                                     _ObjectName)
#endif

}    // namespace Vega
