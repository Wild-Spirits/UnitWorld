#include "VkSwapchain.hpp"

#include "Vega/Core/Application.hpp"

namespace Vega
{

    void VkSwapchain::Create(const VkContext& m_VkContex, const VkDeviceWrapper& _VkDeviceWrapper,
                             VkSurfaceKHR _VkSurface)
    {
        Ref<Window> window = Application::Get().GetWindow();
        VkExtent2D swapchainExtent = { window->GetWidth(), window->GetHeight() };

        VkSwapchainSupportInfo swapchainSupportInfo = _VkDeviceWrapper.GetSwapchainSupportInfo(_VkSurface);

        bool found = false;
        for (uint32_t i = 0; i < swapchainSupportInfo.FormatCount; ++i)
        {
            VkSurfaceFormatKHR format = swapchainSupportInfo.Formats[i];
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_ImageFormat = format;
                found = true;
                break;
            }
        }

        if (!found)
        {
            m_ImageFormat = swapchainSupportInfo.Formats[0];
        }

        // FIFO and MAILBOX support vsync, IMMEDIATE does not.
        // TODO: vsync seems to hold up the game update for some reason.
        // It theoretically should be post-update and pre-render where that happens.
        swapchain->flags = flags;
        VkPresentModeKHR present_mode;
        if (flags & RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT)
        {
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
            // Only try for mailbox mode if not in power-saving mode.
            if ((flags & RENDERER_CONFIG_FLAG_POWER_SAVING_BIT) == 0)
            {
                for (u32 i = 0; i < context->device.swapchain_support.present_mode_count; ++i)
                {
                    VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];
                    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        present_mode = mode;
                        break;
                    }
                }
            }
        }
        else
        {
            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    void VkSwapchain::Recreate() { }

    void VkSwapchain::Destroy() { }

}    // namespace Vega
