#include "VulkanSwapchain.hpp"

#include "Utils/VulkanUtils.hpp"
#include "Vega/Core/Application.hpp"

#include <utility>

namespace Vega
{

    bool VulkanSwapchain::Create(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                                 RendererBackendConfig::Flags _RendererFlags, VkSurfaceKHR _VkSurface,
                                 VkSwapchainKHR _OldSwapchain, bool _LogDetails)
    {
        VkDevice logicalDevice = _VkDeviceWrapper.GetLogicalDevice();

        Ref<Window> window = Application::Get().GetWindow();
        // Physical pixels, not the logical window size: with HiDPI scaling a logical-sized swapchain is
        // smaller than the surface, and the compositor stretches the image (ImGui renders at framebuffer
        // scale, so its output would also be clipped). currentExtent below overrides this when defined.
        glm::uvec2 framebufferSize = window->GetFramebufferSize();
        VkExtent2D swapchainExtent = { framebufferSize.x, framebufferSize.y };

        m_SwapchainSupportInfo = _VkDeviceWrapper.GetSwapchainSupportInfo(_VkSurface);

        // Must be checked before the first Formats[0]/PresentModes[0] use below: an empty list means
        // the selected physical device cannot present to this surface and indexing it is UB.
        if (m_SwapchainSupportInfo.Formats.empty() || m_SwapchainSupportInfo.PresentModes.empty())
        {
            VEGA_CORE_CRITICAL(
                "Vulkan swapchain: surface query returned {} formats and {} present modes for window "
                "\"{}\". The selected physical device cannot present to this surface; check the GPU that "
                "owns the display versus the selected device.",
                m_SwapchainSupportInfo.Formats.size(), m_SwapchainSupportInfo.PresentModes.size(),
                window->GetTitle());
            VEGA_CORE_ASSERT(false, "Swapchain support info is invalid.");
            return false;
        }

        bool found = false;
        for (size_t i = 0; i < m_SwapchainSupportInfo.Formats.size(); ++i)
        {
            VkSurfaceFormatKHR format = m_SwapchainSupportInfo.Formats[i];
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_ImageFormat = format;
                if (_LogDetails)
                {
                    VEGA_CORE_TRACE("Vulkan swapchain: B8G8R8A8_UNORM format selected.");
                }
                found = true;
                break;
            }
        }

        if (!found)
        {
            m_ImageFormat = m_SwapchainSupportInfo.Formats[0];
        }

        // FIFO and MAILBOX support vsync, IMMEDIATE does not.
        // TODO: vsync seems to hold up the game update for some reason.
        // It theoretically should be post-update and pre-render where that happens.
        m_RendererFlags = _RendererFlags;
        VkPresentModeKHR presentMode;
        if (m_RendererFlags & RendererBackendConfig::kRendererConfigFlagVsyncEnabledBit)
        {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
            // Only try for mailbox mode if not in power-saving mode.
            if ((m_RendererFlags & RendererBackendConfig::kRendererConfigFlagPowerSavingBit) == 0)
            {
                for (size_t i = 0; i < m_SwapchainSupportInfo.PresentModes.size(); ++i)
                {
                    VkPresentModeKHR mode = m_SwapchainSupportInfo.PresentModes[i];
                    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        if (_LogDetails)
                        {
                            VEGA_CORE_TRACE("Vulkan swapchain: Mailbox present mode selected.");
                        }
                        presentMode = mode;
                        break;
                    }
                }
            }
            if (presentMode == VK_PRESENT_MODE_FIFO_KHR && _LogDetails)
            {
                VEGA_CORE_TRACE("Vulkan swapchain: FIFO present mode selected.");
            }
        }
        else
        {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            if (_LogDetails)
            {
                VEGA_CORE_TRACE("Vulkan swapchain: Immediate present mode selected.");
            }
        }

        if (m_SwapchainSupportInfo.Capabilities.currentExtent.width != UINT32_MAX)
        {
            swapchainExtent = m_SwapchainSupportInfo.Capabilities.currentExtent;
        }

        VkExtent2D min = m_SwapchainSupportInfo.Capabilities.minImageExtent;
        VkExtent2D max = m_SwapchainSupportInfo.Capabilities.maxImageExtent;
        swapchainExtent.width = glm::clamp(swapchainExtent.width, min.width, max.width);
        swapchainExtent.height = glm::clamp(swapchainExtent.height, min.height, max.height);

        if (swapchainExtent.width == 0 || swapchainExtent.height == 0)
        {
            VEGA_CORE_INFO("Swapchain extent is zero");
            return false;
        }
        m_Extent = swapchainExtent;

        uint32_t imageCount = m_SwapchainSupportInfo.Capabilities.minImageCount + 1;
        if (m_SwapchainSupportInfo.Capabilities.maxImageCount > 0 &&
            imageCount > m_SwapchainSupportInfo.Capabilities.maxImageCount)
        {
            imageCount = m_SwapchainSupportInfo.Capabilities.maxImageCount;
        }

        m_MaxFramesInFlight = imageCount - 1;

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = _VkSurface,
            .minImageCount = imageCount,
            .imageFormat = m_ImageFormat.format,
            .imageColorSpace = m_ImageFormat.colorSpace,
            .imageExtent = swapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .preTransform = m_SwapchainSupportInfo.Capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = _OldSwapchain,
        };

        if (_VkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().GraphicsQueueIndex !=
            _VkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().PresentQueueIndex)
        {
            uint32_t queueFamilyIndices[] = {
                static_cast<uint32_t>(_VkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().GraphicsQueueIndex),
                static_cast<uint32_t>(_VkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().PresentQueueIndex),
            };

            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0;
            swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        }

        VkResult swapchainResult =
            vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, _VkContex.VkAllocator, &m_VkSwapchain);
        if (!VulkanResultIsSuccess(swapchainResult))
        {
            VEGA_CORE_CRITICAL("Failed to create Vulkan swapchain with the error: {}.",
                               VulkanResultString(swapchainResult, true));
            VEGA_CORE_ASSERT(false, "Failed to create Vulkan swapchain!");
            return false;
        }

        imageCount = 0;
        VkResult getImageResult = vkGetSwapchainImagesKHR(logicalDevice, m_VkSwapchain, &imageCount, nullptr);
        if (!VulkanResultIsSuccess(getImageResult))
        {
            VEGA_CORE_CRITICAL("Failed to obtain image count from Vulkan swapchain with the error: {}.",
                               VulkanResultString(getImageResult, true));
            VEGA_CORE_ASSERT(false, "Failed to obtain image count from Vulkan swapchain!");
            VEGA_CORE_ASSERT(false);
            return false;
        }

        m_SwapchainImages.resize(imageCount);
        getImageResult = vkGetSwapchainImagesKHR(logicalDevice, m_VkSwapchain, &imageCount, m_SwapchainImages.data());
        if (!VulkanResultIsSuccess(getImageResult))
        {
            VEGA_CORE_CRITICAL("Failed to obtain image count from Vulkan swapchain with the error: {}.",
                               VulkanResultString(getImageResult, true));
            VEGA_CORE_ASSERT(false);
            return false;
        }

        m_VulkanColorBufferTextures.clear();
        m_VulkanColorBufferTextures.reserve(imageCount);
        m_ColorBufferTextures.clear();
        m_ColorBufferTextures.reserve(imageCount);
        for (size_t i = 0; i < imageCount; ++i)
        {
            Ref<VulkanTexture> colorBufferTexture = CreateRef<VulkanTexture>();
            colorBufferTexture->CreateSwapchainTexture(m_SwapchainImages[i], m_ImageFormat,
                                                       TextureProps {
                                                           .Type = TextureProps::TextureType::k2D,
                                                           .Width = swapchainExtent.width,
                                                           .Height = swapchainExtent.height,
                                                           .MipLevels = 1,
                                                           .ArraySize = 1,
                                                       });
            m_VulkanColorBufferTextures.push_back(colorBufferTexture);
            m_ColorBufferTextures.push_back(colorBufferTexture);
        }

        if (_LogDetails)
        {
            VEGA_CORE_TRACE("Swapchain created successfully.");
        }

        return true;
    }

    bool VulkanSwapchain::Recreate(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                                   RendererBackendConfig::Flags _RendererFlags, VkSurfaceKHR _VkSurface,
                                   bool _LogDetails)
    {
        // Hand the old swapchain to the new one via oldSwapchain so the driver can keep presenting until the
        // new swapchain is ready: destroying it first opened a presentation gap that made the window frame
        // jitter during interactive resize. The retired views/handle are destroyed only after creation.
        VkSwapchainKHR oldSwapchain = m_VkSwapchain;
        std::vector<Ref<VulkanTexture>> oldTextures = std::move(m_VulkanColorBufferTextures);
        m_VulkanColorBufferTextures.clear();
        m_ColorBufferTextures.clear();
        m_SwapchainImages.clear();
        m_VkSwapchain = VK_NULL_HANDLE;

        bool result = Create(_VkContex, _VkDeviceWrapper, _RendererFlags, _VkSurface, oldSwapchain, _LogDetails);

        DestroyRetired(_VkContex, _VkDeviceWrapper, oldSwapchain, oldTextures);

        return result;
    }

    void VulkanSwapchain::Destroy(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper)
    {
        // Caller must guarantee the device is idle: RecreateSwapchain waits idle before recreating, and
        // OnWindowDestroy is preceded by Application::WaitIdle. A second vkDeviceWaitIdle here stalled the
        // whole device twice per resize frame.
        DestroyRetired(_VkContex, _VkDeviceWrapper, m_VkSwapchain, m_VulkanColorBufferTextures);

        m_VulkanColorBufferTextures.clear();
        m_ColorBufferTextures.clear();
        m_SwapchainImages.clear();
        m_VkSwapchain = VK_NULL_HANDLE;
    }

    void VulkanSwapchain::DestroyRetired(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                                         VkSwapchainKHR _OldSwapchain,
                                         const std::vector<Ref<VulkanTexture>>& _OldTextures)
    {
        VkDevice logicalDevice = _VkDeviceWrapper.GetLogicalDevice();

        // Only the image views are ours; the images belong to the swapchain, so do not call OnDetach here
        for (const Ref<VulkanTexture>& texture : _OldTextures)
        {
            vkDestroyImageView(logicalDevice, texture->GetTextureVkImageView(), _VkContex.VkAllocator);
        }

        if (_OldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(logicalDevice, _OldSwapchain, _VkContex.VkAllocator);
        }
    }

}    // namespace Vega
