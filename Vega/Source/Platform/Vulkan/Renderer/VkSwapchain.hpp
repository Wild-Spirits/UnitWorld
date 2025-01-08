#pragma once

#include "VkBase.hpp"
#include "VkDeviceWrapper.hpp"

namespace Vega
{

    class VkSwapchain
    {
    public:
        VkSwapchain();

        void Create(const VkContext& m_VkContex, const VkDeviceWrapper& _VkDeviceWrapper, VkSurfaceKHR _VkSurface);
        void Recreate();
        void Destroy();

    private:
        VkSurfaceFormatKHR m_ImageFormat;
    };

}    // namespace Vega
