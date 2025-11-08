#include "VulkanSwapchain.hpp"

#include "Utils/VulkanUtils.hpp"
#include "Vega/Core/Application.hpp"

namespace Vega
{

    bool VulkanSwapchain::Create(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                                 RendererBackendConfig::Flags _RendererFlags, VkSurfaceKHR _VkSurface)
    {
        VkDevice logicalDevice = _VkDeviceWrapper.GetLogicalDevice();

        Ref<Window> window = Application::Get().GetWindow();
        VkExtent2D swapchainExtent = { window->GetWidth(), window->GetHeight() };

        m_SwapchainSupportInfo = _VkDeviceWrapper.GetSwapchainSupportInfo(_VkSurface);

        bool found = false;
        for (size_t i = 0; i < m_SwapchainSupportInfo.Formats.size(); ++i)
        {
            VkSurfaceFormatKHR format = m_SwapchainSupportInfo.Formats[i];
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_ImageFormat = format;
                VEGA_CORE_TRACE("Vulkan swapchain: B8G8R8A8_UNORM format selected.");
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
                        VEGA_CORE_INFO("Vulkan swapchain: Mailbox present mode selected.");
                        presentMode = mode;
                        break;
                    }
                }
            }
            if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
            {
                VEGA_CORE_INFO("Vulkan swapchain: FIFO present mode selected.");
            }
        }
        else
        {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            VEGA_CORE_INFO("Vulkan swapchain: Immediate present mode selected.");
        }

        if (m_SwapchainSupportInfo.Formats.size() < 1 || m_SwapchainSupportInfo.PresentModes.size() < 1)
        {
            VEGA_CORE_INFO("Required swapchain support not present, skipping device.");
            VEGA_CORE_ASSERT(false, "Swapchain support info is invalid.");
            return false;
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
            .oldSwapchain = nullptr,
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

        VEGA_CORE_INFO("Swapchain created successfully.");

        return true;
    }

    bool VulkanSwapchain::Recreate(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper,
                                   RendererBackendConfig::Flags _RendererFlags, VkSurfaceKHR _VkSurface)
    {
        Destroy(_VkContex, _VkDeviceWrapper);
        return Create(_VkContex, _VkDeviceWrapper, _RendererFlags, _VkSurface);
    }

    void VulkanSwapchain::Destroy(const VulkanContext& _VkContex, const VulkanDeviceWrapper& _VkDeviceWrapper)
    {
        VkDevice logicalDevice = _VkDeviceWrapper.GetLogicalDevice();

        vkDeviceWaitIdle(logicalDevice);

        for (auto& texture : m_VulkanColorBufferTextures)
        {
            vkDestroyImageView(logicalDevice, texture->GetTextureVkImageView(), _VkContex.VkAllocator);
        }

        vkDestroySwapchainKHR(logicalDevice, m_VkSwapchain, _VkContex.VkAllocator);
    }

}    // namespace Vega
