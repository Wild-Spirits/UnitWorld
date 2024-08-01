#pragma once

#include <string>

#include <vulkan/vulkan.h>

namespace LM
{

    bool VkResultIsSuccess(VkResult _Result);

    std::string VkResultString(VkResult _Result, bool _GetExtended);

}    // namespace LM
