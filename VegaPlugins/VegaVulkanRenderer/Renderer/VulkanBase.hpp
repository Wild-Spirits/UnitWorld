#pragma once

#include "Vega/Core/Assert.hpp"

#include <vulkan/vulkan.h>

struct shaderc_compiler;

namespace Vega
{

    struct VulkanContext
    {
        VkInstance VkInstance;
        VkAllocationCallbacks* VkAllocator = nullptr;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT VkDebugMessenger;
        PFN_vkSetDebugUtilsObjectNameEXT PfnSetDebugUtilsObjectNameEXT;
        PFN_vkSetDebugUtilsObjectTagEXT PfnSetDebugUtilsObjectTagEXT;
        PFN_vkCmdBeginDebugUtilsLabelEXT PfnCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT PfnCmdEndDebugUtilsLabelEXT;
#endif

        PFN_vkCmdSetPrimitiveTopologyEXT VkCmdSetPrimitiveTopologyEXT;
        PFN_vkCmdSetFrontFaceEXT VkCmdSetFrontFaceEXT;
        PFN_vkCmdSetStencilTestEnableEXT VkCmdSetStencilTestEnableEXT;
        PFN_vkCmdSetDepthTestEnableEXT VkCmdSetDepthTestEnableEXT;
        PFN_vkCmdSetDepthWriteEnableEXT VkCmdSetDepthWriteEnableEXT;
        PFN_vkCmdSetStencilOpEXT VkCmdSetStencilOpEXT;

        PFN_vkCmdBeginRenderingKHR VkCmdBeginRenderingKHR;
        PFN_vkCmdEndRenderingKHR VkCmdEndRenderingKHR;

        shaderc_compiler* ShaderCompiler;
    };

    typedef uint32_t VulkanDeviceSupportFlags;

    namespace VulkanDeviceSupportFlagBits
    {

        enum DeviceSupportFlagBits : VulkanDeviceSupportFlags
        {
            kNoneBit = 0x00,
            kNativeDynamicStateBit = 0x01,
            kDynamicStateBit = 0x02,
            kLineSmoothRasterizationBit = 0x04
        };

    }    // namespace VulkanDeviceSupportFlagBits

#define VK_CHECK(_Expr)                                                                                                \
    {                                                                                                                  \
        VkResult exprRes = _Expr;                                                                                      \
        VEGA_CORE_ASSERT(exprRes == VK_SUCCESS);                                                                       \
    }

}    // namespace Vega
