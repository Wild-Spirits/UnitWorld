#pragma once

#include "Vega/Core/Assert.hpp"

#include <vulkan/vulkan.h>

#define VK_CHECK(_Expr)                                                                                                \
    {                                                                                                                  \
        VkResult exprRes = _Expr;                                                                                      \
        LM_CORE_ASSERT(exprRes == VK_SUCCESS);                                                                         \
    }

namespace VkDeviceSupportFlagBits
{

    enum DeviceSupportFlagBits : uint32_t
    {
        kNoneBit = 0x00,
        kNativeDynamicStateBit = 0x01,
        kDynamicStateBit = 0x02,
        kLineSmoothRasterisationBit = 0x04
    };

}    // namespace VkDeviceSupportFlagBits
