#include "VulkanRendererBackend.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/Utils/Log.hpp"

#include "Platform/Platform.hpp"
#include "VulkanFrameBuffer.hpp"
#include "VulkanRenderBuffer.hpp"
#include "VulkanShader.hpp"
#include "VulkanTexture.hpp"
#include <memory>

#ifdef VEGA_PLATFORM_DESKTOP
    #include "GLFW/glfw3.h"
#endif

#include "ImGui/VulkanImGuiImpl.hpp"
#include "Platform/VulkanPlatform.hpp"
#include "Utils/VulkanUtils.hpp"

#include <shaderc/shaderc.h>

namespace Vega
{

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                       VkDebugUtilsMessageTypeFlagsEXT message_types,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                       void* _UserData);

    VulkanRendererBackend::VulkanRendererBackend() { m_Instance = this; }

    bool VulkanRendererBackend::Init()
    {
        VEGA_CORE_TRACE("Initialize VulkanRendererBackend");

        // NOTE: is it hack?
#ifdef VEGA_PLATFORM_DESKTOP
        glfwInit();
#endif

        m_VkContext.VkAllocator = nullptr;

        uint32_t vkApiVersion = 0;
        vkEnumerateInstanceVersion(&vkApiVersion);

        uint32_t vkApiVersionMajor = VK_API_VERSION_MAJOR(vkApiVersion);
        uint32_t vkApiVersionMinor = VK_API_VERSION_MINOR(vkApiVersion);
        uint32_t vkApiVersionPatch = VK_API_VERSION_PATCH(vkApiVersion);
        VEGA_CORE_TRACE("Vulkan API Version: {}.{}.{}", vkApiVersionMajor, vkApiVersionMinor, vkApiVersionPatch);

        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Lumen",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "Vega",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_MAKE_API_VERSION(0, vkApiVersionMajor, vkApiVersionMinor, vkApiVersionPatch),
        };

        std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
        VulkanGetPlatformRequiredExtensionNames(&instanceExtensions);
#ifdef _DEBUG
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        VerifyRequiredExtensions(instanceExtensions);

        std::vector<const char*> requiredValidationLayers;
#ifdef _DEBUG
        requiredValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

        VkInstanceCreateInfo instanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size()),
            .ppEnabledLayerNames = requiredValidationLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
            .ppEnabledExtensionNames = instanceExtensions.data(),
        };

#ifdef VEGA_PLATFORM_APPLE
        instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        VkResult instanceResult = vkCreateInstance(&instanceCreateInfo, nullptr, &m_VkContext.VkInstance);
        if (!VulkanResultIsSuccess(instanceResult))
        {
            VEGA_CORE_CRITICAL("Vulkan instance creation failed with result: {}",
                               VulkanResultString(instanceResult, true));
            VEGA_CORE_ASSERT(false, "Vulkan instance creation failed!");
            return false;
        }

        VEGA_CORE_TRACE("Vulkan instance created");

#ifdef _DEBUG
        VEGA_CORE_TRACE("Creating Vulkan debugger...");
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
            .pfnUserCallback = VulkanDebugCallback
        };

        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_VkContext.VkInstance, "vkCreateDebugUtilsMessengerEXT");
        VEGA_CORE_ASSERT(func, "Failed to create debug messenger!");
        VK_CHECK(
            func(m_VkContext.VkInstance, &debugCreateInfo, m_VkContext.VkAllocator, &m_VkContext.VkDebugMessenger));
        VEGA_CORE_TRACE("Vulkan debugger created.");

        m_VkContext.PfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
            m_VkContext.VkInstance, "vkSetDebugUtilsObjectNameEXT");
        if (!m_VkContext.PfnSetDebugUtilsObjectNameEXT)
        {
            VEGA_CORE_WARN("Unable to load function pointer for vkSetDebugUtilsObjectNameEXT. "
                           "Debug functions associated with this will not work.");
        }
        m_VkContext.PfnSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(
            m_VkContext.VkInstance, "vkSetDebugUtilsObjectTagEXT");
        if (!m_VkContext.PfnSetDebugUtilsObjectTagEXT)
        {
            VEGA_CORE_WARN("Unable to load function pointer for vkSetDebugUtilsObjectTagEXT. "
                           "Debug functions associated with this will not work.");
        }
        m_VkContext.PfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            m_VkContext.VkInstance, "vkCmdBeginDebugUtilsLabelEXT");
        if (!m_VkContext.PfnCmdBeginDebugUtilsLabelEXT)
        {
            VEGA_CORE_WARN("Unable to load function pointer for vkCmdBeginDebugUtilsLabelEXT. "
                           "Debug functions associated with this will not work.");
        }

        m_VkContext.PfnCmdEndDebugUtilsLabelEXT =
            (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_VkContext.VkInstance, "vkCmdEndDebugUtilsLabelEXT");
        if (!m_VkContext.PfnCmdEndDebugUtilsLabelEXT)
        {
            VEGA_CORE_WARN("Unable to load function pointer for vkCmdEndDebugUtilsLabelEXT. "
                           "Debug functions associated with this will not work.");
        }
#endif

        if (!m_VkDeviceWrapper.Init(m_VkContext))
        {
            VEGA_CORE_CRITICAL("Failed to initialize VulkanDeviceWrapper");
            return false;
        }

        m_VkContext.ShaderCompiler = shaderc_compiler_initialize();

        return true;
    }

    void VulkanRendererBackend::Shutdown()
    {
        VkDevice logicalDevice = m_VkDeviceWrapper.GetLogicalDevice();

        vkDeviceWaitIdle(logicalDevice);

        DestroyImGuiDescriptorPool();

        if (m_VkContext.ShaderCompiler)
        {
            shaderc_compiler_release(m_VkContext.ShaderCompiler);
        }

        VEGA_CORE_TRACE("Destroying Vulkan device...");
        m_VkDeviceWrapper.Shutdown(m_VkContext);

#ifdef _DEBUG
        VEGA_CORE_TRACE("Destroying Vulkan debugger...");
        if (m_VkContext.VkDebugMessenger)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                m_VkContext.VkInstance, "vkDestroyDebugUtilsMessengerEXT");
            func(m_VkContext.VkInstance, m_VkContext.VkDebugMessenger, m_VkContext.VkAllocator);
        }
#endif

        VEGA_CORE_TRACE("Destroying Vulkan instance...");
        vkDestroyInstance(m_VkContext.VkInstance, m_VkContext.VkAllocator);

        // NOTE: is it hack?
#ifdef VEGA_PLATFORM_DESKTOP
        glfwTerminate();
#endif
    }

    // TODO: Add per window resources to be able to create multiple windows (not imgui)
    bool VulkanRendererBackend::OnWindowCreate(Ref<Window> _Window)
    {
        VkDevice logicalDevice = m_VkDeviceWrapper.GetLogicalDevice();
        std::string_view windowTitle = _Window->GetTitle();

        VEGA_CORE_TRACE("Creating Vulkan surface for window {}", _Window->GetTitle());

        // TODO: move to Platform code
#ifdef VEGA_PLATFORM_DESKTOP
        VkResult surfaceResult =
            glfwCreateWindowSurface(m_VkContext.VkInstance, static_cast<GLFWwindow*>(_Window->GetNativeWindow()),
                                    m_VkContext.VkAllocator, &m_VkSurface);

        if (surfaceResult != VK_SUCCESS)
        {
            VEGA_CORE_CRITICAL("Failed to create platform surface for window {} with error: {}", windowTitle,
                               VulkanResultString(surfaceResult, true));
            VEGA_CORE_ASSERT(false, "Failed to create platform surface!");
            return false;
        }
#endif

        VEGA_CORE_TRACE("Vulkan surface created for window {}", windowTitle);

        // TODO: Add m_RendererFlags to upper level of call
        m_VkSwapchain.Create(m_VkContext, m_VkDeviceWrapper, m_RendererFlags, m_VkSurface);

        // May need to move this code to the Window and/or store this data in the Window class
        {
            uint32_t maxFramesInFlight = m_VkSwapchain.GetMaxFramesInFlight();

            m_ImageAvailableSemaphores.resize(maxFramesInFlight);
            m_QueueCompleteSemaphores.resize(maxFramesInFlight);
            m_InFlightFences.resize(maxFramesInFlight);

            m_StagingBuffers.reserve(maxFramesInFlight);
            for (uint32_t i = 0; i < maxFramesInFlight; ++i)
            {
                VkSemaphoreCreateInfo semaphoreCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                };

                VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, m_VkContext.VkAllocator,
                                           &m_ImageAvailableSemaphores[i]));
                VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, m_VkContext.VkAllocator,
                                           &m_QueueCompleteSemaphores[i]));

                VkFenceCreateInfo fenceCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                };

                VK_CHECK(vkCreateFence(logicalDevice, &fenceCreateInfo, m_VkContext.VkAllocator, &m_InFlightFences[i]));

                m_StagingBuffers.emplace_back(CreateRef<VulkanRenderBuffer>(RenderBufferProps {
                    .Name = std::format("{}_staging_buffer_{}", windowTitle, i),
                    .Type = RenderBufferType::kStaging,
                    .ElementSize = 1,
                    .ElementCount = 256 * 1024 * 1024,
                    .AllocatorType = RenderBufferAllocatorType::kLinear,
                }));
            }
        }

        CreateGraphicsCommandBuffer(_Window);

        VEGA_CORE_TRACE("Creating Vulkan depth buffer for window {} . . .", windowTitle);
        m_DepthBufferTextures.clear();
        m_DepthBufferTextures.reserve(m_VkSwapchain.GetImagesCount());
        for (size_t i = 0; i < m_VkSwapchain.GetImagesCount(); ++i)
        {
            Ref<VulkanTexture> depthBufferTexture = CreateRef<VulkanTexture>();
            depthBufferTexture->VulkanCreate(
                std::format("{}_depth_buffer_{}", windowTitle, i),
                TextureProps {
                    .Type = TextureProps::TextureType::k2D,
                    .Width = _Window->GetWidth(),
                    .Height = _Window->GetHeight(),
                    .ChannelCount = m_VkDeviceWrapper.GetDepthChannelCount(),
                    .MipLevels = 1,
                    .ArraySize = 1,
                },
                m_VkDeviceWrapper.GetDepthFormat(), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            m_DepthBufferTextures.push_back(depthBufferTexture);
        }

        VEGA_CORE_INFO("Vulkan depth buffer created successfully.");

        return true;
    }

    void VulkanRendererBackend::OnWindowDestroy(Ref<Window> _Window)
    {
        VkDevice logicalDevice = m_VkDeviceWrapper.GetLogicalDevice();

        for (Ref<VulkanRenderBuffer> stagingBuffer : m_StagingBuffers)
        {
            stagingBuffer->Clear();
            stagingBuffer->Destroy();
        }

        for (VkSemaphore semaphore : m_ImageAvailableSemaphores)
        {
            vkDestroySemaphore(logicalDevice, semaphore, m_VkContext.VkAllocator);
        }
        m_ImageAvailableSemaphores.clear();

        for (VkSemaphore semaphore : m_QueueCompleteSemaphores)
        {
            vkDestroySemaphore(logicalDevice, semaphore, m_VkContext.VkAllocator);
        }
        m_QueueCompleteSemaphores.clear();

        for (VkFence fence : m_InFlightFences)
        {
            vkDestroyFence(logicalDevice, fence, m_VkContext.VkAllocator);
        }
        m_InFlightFences.clear();

        for (VkCommandBuffer commandBuffer : m_GraphicsCommandBuffer)
        {
            vkFreeCommandBuffers(logicalDevice, m_VkDeviceWrapper.GetGraphicsCommandPool(), 1, &commandBuffer);
        }
        m_GraphicsCommandBuffer.clear();

        for (Ref<VulkanTexture> depthBufferTexture : m_DepthBufferTextures)
        {
            depthBufferTexture->Destroy();
        }
        m_DepthBufferTextures.clear();

        m_VkSwapchain.Destroy(m_VkContext, m_VkDeviceWrapper);

        vkDestroySurfaceKHR(m_VkContext.VkInstance, m_VkSurface, m_VkContext.VkAllocator);
        m_VkSurface = nullptr;
    }

    bool VulkanRendererBackend::OnResize()
    {
        m_IsNeedRecreateSwapchain = true;
        return true;
    }

    bool VulkanRendererBackend::FramePrepareWindowSurface()
    {
        // TODO: Implement spawchain recreation

        VkDevice logicalDevice = m_VkDeviceWrapper.GetLogicalDevice();

        if (m_IsNeedRecreateSwapchain)
        {
            VEGA_CORE_TRACE("m_IsNeedRecreateSwapchain");
            VkResult deviceIdleResult = vkDeviceWaitIdle(logicalDevice);
            if (!VulkanResultIsSuccess(deviceIdleResult))
            {
                VEGA_CORE_CRITICAL("FramePrepareWindowSurface vkDeviceWaitIdle failed: {}",
                                   VulkanResultString(deviceIdleResult, true));
                VEGA_CORE_ASSERT(false, "FramePrepareWindowSurface vkDeviceWaitIdle failed!");
                return false;
            }

            if (!RecreateSwapchain())
            {
                VEGA_CORE_TRACE("Failed to recreate swapchain!");
                // VEGA_CORE_ASSERT(false, "Failed to recreate swapchain!");
                return false;
            }
            // TODO: Recreate depth buffer texture

            if (m_VkSwapchain.GetImagesCount() != m_DepthBufferTextures.size())
            {
                // TODO: Need to destroy and recreate depth buffer textures array on swapchain images count change
                VEGA_CORE_WARN(
                    "TODO: Need to destroy and recreate depth buffer textures array on swapchain images count change");
            }
            Ref<Window> window = Application::Get().GetWindow();
            for (size_t i = 0; i < m_VkSwapchain.GetImagesCount(); ++i)
            {
                Ref<VulkanTexture> depthBufferTexture = m_DepthBufferTextures[i];
                depthBufferTexture->Resize(std::format("{}_depth_buffer_{}", window->GetTitle(), i), window->GetWidth(),
                                           window->GetHeight());
            }

            m_IsNeedRecreateSwapchain = false;
            return false;
        }

        VkResult inFlightResult =
            vkWaitForFences(logicalDevice, 1, &m_InFlightFences[m_CurrentFrame], true, UINT64_MAX);
        if (!VulkanResultIsSuccess(inFlightResult))
        {
            VEGA_CORE_CRITICAL("In-flight fence wait failure! error: {}", VulkanResultString(inFlightResult, true));
            VEGA_CORE_ASSERT(false, "In-flight fence wait failure!");
            return false;
        }

        VkResult acquireNextImageResult =
            vkAcquireNextImageKHR(logicalDevice, m_VkSwapchain.GetSwapchainHandle(), UINT64_MAX,
                                  m_ImageAvailableSemaphores[m_CurrentFrame], 0, &m_ImageIndex);

        if (acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            VEGA_CORE_TRACE("Recreateing swapchain");
            if (!m_VkSwapchain.Recreate(m_VkContext, m_VkDeviceWrapper, m_RendererFlags, m_VkSurface))
            {
                VEGA_CORE_ASSERT(false, "Failed to recreate swapchain!");
            }
            return false;
        }
        else if (acquireNextImageResult != VK_SUCCESS && acquireNextImageResult != VK_SUBOPTIMAL_KHR)
        {
            VEGA_CORE_ASSERT(false, "Failed to acquire swapchain image!");
            return false;
        }

        VK_CHECK(vkResetFences(logicalDevice, 1, &m_InFlightFences[m_CurrentFrame]));

        m_StagingBuffers[m_CurrentFrame]->Clear();

        return true;
    }

    void VulkanRendererBackend::FrameCommandListBegin()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        CommandBufferReset(commandBuffer);
        CommandBufferBegin(commandBuffer, false, false, false);

        SetWinding();

        SetStencilReference(0);
        SetStencilCompareMask(0xFF);
        SetStencilOp(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS);
        SetStencilTestEnabled(false);

        SetDepthTestEnabled(true);
        SetDepthWriteEnabled(true);

        SetStencilWriteMask(0x00);

        ClearColorTexture();
    }

    void VulkanRendererBackend::FrameCommandListEnd()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        CommandBufferEnd(commandBuffer);
    }

    void VulkanRendererBackend::FrameSubmit()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        VkPipelineStageFlags stageFlags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame],
            .pWaitDstStageMask = stageFlags,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_QueueCompleteSemaphores[m_CurrentFrame],
        };

        VkResult result =
            vkQueueSubmit(m_VkDeviceWrapper.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]);
        if (result != VK_SUCCESS)
        {
            VEGA_CORE_CRITICAL("vkQueueSubmit failed with result: {}", VulkanResultString(result, true));
            VEGA_CORE_ASSERT(false, "vkQueueSubmit failed!");
        }

        CommandBufferUpdateSubmited(commandBuffer);
    }

    void VulkanRendererBackend::TmpRendergraphExecute() { ColorTexturePepareForPresent(); }

    void VulkanRendererBackend::FramePresent()
    {
        VkSwapchainKHR swapchain = m_VkSwapchain.GetSwapchainHandle();

        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_QueueCompleteSemaphores[m_CurrentFrame],
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &m_ImageIndex,
            .pResults = 0,
        };

        VkResult result = vkQueuePresentKHR(m_VkDeviceWrapper.GetPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            if (!m_VkSwapchain.Recreate(m_VkContext, m_VkDeviceWrapper, m_RendererFlags, m_VkSurface))
            {
                VEGA_CORE_WARN("Failed to recreate swapchain after presentation");
            }
            else
            {
                VEGA_CORE_TRACE("Swapchain recreated because swapchain returned out of date or suboptimal.");
            }
        }
        else if (result != VK_SUCCESS)
        {
            VEGA_CORE_ASSERT(false, "Failed to present swap chain image!");
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % m_VkSwapchain.GetMaxFramesInFlight();
    }

    void VulkanRendererBackend::ClearColorTexture()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        Ref<VulkanTexture> colorTexture = GetCurrentColorTexture();

        uint32_t graphicsQueueIndex =
            static_cast<uint32_t>(m_VkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().GraphicsQueueIndex);

        VkImageSubresourceRange barrierSubresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = colorTexture->GetMipLevels(),
            .baseArrayLayer = 0,
            .layerCount = colorTexture->GetArraySize(),
        };

        VkImageMemoryBarrier barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = graphicsQueueIndex,
            .dstQueueFamilyIndex = graphicsQueueIndex,
            .image = colorTexture->GetTextureVkImage(),
            .subresourceRange = barrierSubresourceRange,
        };

        VkClearColorValue clearColorValue {
            { 1.0f, 0.0f, 0.0f, 1.0f }
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0,
                             0, 0, 1, &barrier);

        const VkImageSubresourceRange& imageSubresourceRange = colorTexture->GetImageViewCreateInfo().subresourceRange;

        vkCmdClearColorImage(commandBuffer,                           //
                             colorTexture->GetTextureVkImage(),       //
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             &clearColorValue,                        //
                             colorTexture->GetArraySize(),            //
                             &imageSubresourceRange);
    }

    void VulkanRendererBackend::ColorTexturePepareForPresent()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        Ref<VulkanTexture> colorTexture = GetCurrentColorTexture();

        uint32_t graphicsQueueIndex =
            static_cast<uint32_t>(m_VkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().GraphicsQueueIndex);

        VkImageSubresourceRange barrierSubresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = colorTexture->GetMipLevels(),
            .baseArrayLayer = 0,
            .layerCount = colorTexture->GetArraySize(),
        };

        VkImageMemoryBarrier barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = graphicsQueueIndex,
            .dstQueueFamilyIndex = graphicsQueueIndex,
            .image = colorTexture->GetTextureVkImage(),
            .subresourceRange = barrierSubresourceRange,
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, 0, 0, 0, 1, &barrier);
    }

    bool VulkanRendererBackend::RecreateSwapchain()
    {
        VkDevice logicalDevice = m_VkDeviceWrapper.GetLogicalDevice();
        auto _Window = Application::Get().GetWindow();

        if (_Window->GetWidth() == 0 || _Window->GetHeight() == 0)
        {
            return false;
        }

        vkDeviceWaitIdle(logicalDevice);

        m_VkDeviceWrapper.DetectDepthFormat();

        if (!m_VkSwapchain.Recreate(m_VkContext, m_VkDeviceWrapper, m_RendererFlags, m_VkSurface))
        {
            VEGA_CORE_ASSERT(false, "Failed to recreate swapchain!");
            return false;
        }

        for (auto& graphicsCommandBuffer : m_GraphicsCommandBuffer)
        {
            if (graphicsCommandBuffer)
            {
                vkFreeCommandBuffers(logicalDevice, m_VkDeviceWrapper.GetGraphicsCommandPool(), 1,
                                     &graphicsCommandBuffer);
            }
        }
        m_GraphicsCommandBuffer.clear();

        CreateGraphicsCommandBuffer(_Window);

        return true;
    }

    bool VulkanRendererBackend::CreateGraphicsCommandBuffer(Ref<Window> _Window)
    {
        VkDevice logicalDevice = m_VkDeviceWrapper.GetLogicalDevice();
        std::string_view windowTitle = _Window->GetTitle();

        m_GraphicsCommandBuffer.resize(m_VkSwapchain.GetImagesCount());
        for (size_t i = 0; i < m_GraphicsCommandBuffer.size(); ++i)
        {
            VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = m_VkDeviceWrapper.GetGraphicsCommandPool(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &m_GraphicsCommandBuffer[i]));

            VK_SET_DEBUG_OBJECT_NAME(m_VkContext.PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                                     VK_OBJECT_TYPE_COMMAND_BUFFER, m_GraphicsCommandBuffer[i],
                                     std::format("{}_command_buffer_{}", windowTitle, i).c_str());
        }
        VEGA_CORE_TRACE("Vulkan command buffers created.");

        return true;
    }

    void VulkanRendererBackend::SetWinding()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        VkFrontFace vkWinding = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdSetFrontFace(commandBuffer, vkWinding);
        }
        else if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit)
        {
            m_VkContext.VkCmdSetFrontFaceEXT(commandBuffer, vkWinding);
        }
        else
        {
            VEGA_CORE_ASSERT(false, "SetWinding cannot be used on a device without dynamic state support!");
        }
    }

    void VulkanRendererBackend::SetStencilReference(uint32_t _StencilReference)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, _StencilReference);
    }

    void VulkanRendererBackend::SetStencilCompareMask(uint32_t _StencilCompareMask)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, _StencilCompareMask);
    }

    void VulkanRendererBackend::SetStencilOp(VkStencilOp _FailOp, VkStencilOp _PassOp, VkStencilOp _DepthFailOp,
                                             VkCompareOp _CompareOp)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, _FailOp, _PassOp, _DepthFailOp,
                              _CompareOp);
        }
        else if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit)
        {
            m_VkContext.VkCmdSetStencilOpEXT(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, _FailOp, _PassOp,
                                             _DepthFailOp, _CompareOp);
        }
        else
        {
            VEGA_CORE_ASSERT(false, "SetStencilOp cannot be used on a device without dynamic state support!");
        }
    }

    void VulkanRendererBackend::SetStencilWriteMask(uint32_t _StencilWriteMask)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        vkCmdSetStencilWriteMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, _StencilWriteMask);
    }

    void VulkanRendererBackend::SetStencilTestEnabled(bool _StencilTestEnabled)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdSetStencilTestEnable(commandBuffer, static_cast<VkBool32>(_StencilTestEnabled));
        }
        else if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit)
        {
            m_VkContext.VkCmdSetStencilTestEnableEXT(commandBuffer, static_cast<VkBool32>(_StencilTestEnabled));
        }
        else
        {
            VEGA_CORE_ASSERT(false, "SetStencilWriteMask cannot be used on a device without dynamic state support!");
        }
    }

    void VulkanRendererBackend::SetDepthTestEnabled(bool _DepthTestEnabled)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdSetDepthTestEnable(commandBuffer, static_cast<VkBool32>(_DepthTestEnabled));
        }
        else if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit)
        {
            m_VkContext.VkCmdSetDepthTestEnableEXT(commandBuffer, static_cast<VkBool32>(_DepthTestEnabled));
        }
        else
        {
            VEGA_CORE_ASSERT(false, "SetDepthTestEnabled cannot be used on a device without dynamic state support!");
        }
    }

    void VulkanRendererBackend::SetDepthWriteEnabled(bool _DepthWriteEnabled)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdSetDepthWriteEnable(commandBuffer, static_cast<VkBool32>(_DepthWriteEnabled));
        }
        else if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit)
        {
            m_VkContext.VkCmdSetDepthWriteEnableEXT(commandBuffer, static_cast<VkBool32>(_DepthWriteEnabled));
        }
        else
        {
            VEGA_CORE_ASSERT(false, "SetDepthWriteEnabled cannot be used on a device without dynamic state support!");
        }
    }

    VkCommandBuffer VulkanRendererBackend::GetCurrentGraphicsCommandBuffer() const
    {
        return m_GraphicsCommandBuffer[m_CurrentFrame];
    }

    Ref<VulkanRenderBuffer> VulkanRendererBackend::GetCurrentStagingBuffer() const
    {
        return m_StagingBuffers[m_CurrentFrame];
    }

    void VulkanRendererBackend::BeginRendering(const glm::ivec2& _ViewportOffset, const glm::uvec2& _ViewportSize,
                                               Ref<FrameBuffer> _FrameBuffer)
    {
        Ref<VulkanFrameBuffer> vulkanFrameBuffer = StaticRefCast<VulkanFrameBuffer>(_FrameBuffer);
        VulkanBeginRendering(_ViewportOffset, _ViewportSize, vulkanFrameBuffer->GetVulkanTextures(),
                             vulkanFrameBuffer->GetVulkanDepthTextures());
    }

    void VulkanRendererBackend::TestFoo()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void VulkanRendererBackend::EndRendering() { VulkanEndRendering(); }

    void VulkanRendererBackend::VulkanBeginRendering(
        const glm::ivec2& _ViewportOffset, const glm::uvec2& _ViewportSize,
        const std::vector<std::vector<Ref<VulkanTexture>>>& _ColorTargets,
        const std::vector<std::vector<Ref<VulkanTexture>>>& _DepthStencilTargets)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        VkRenderingInfo renderInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { 
                .offset = { _ViewportOffset.x, _ViewportOffset.y },
                .extent = { _ViewportSize.x, _ViewportSize.y }, 
            },
            .layerCount = 1,
        };

        VkRenderingAttachmentInfoKHR depth_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        };

        if (_DepthStencilTargets.size() > 0)
        {
            // TODO: Add real depthStencilTargets
        }
        else
        {
            renderInfo.pDepthAttachment = nullptr;
            renderInfo.pStencilAttachment = nullptr;
        }

        renderInfo.colorAttachmentCount = static_cast<uint32_t>(_ColorTargets.size());
        std::vector<VkRenderingAttachmentInfo> colorAttachments;
        if (_ColorTargets.size() > 0)
        {
            // TODO: Create buffer for color attachments to reduce memory allocations per frame
            colorAttachments.reserve(_ColorTargets.size());
            for (size_t i = 0; i < _ColorTargets.size(); ++i)
            {
                VkRenderingAttachmentInfo attachmentInfo {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .pNext = VK_NULL_HANDLE,
                    .imageView = _ColorTargets[i][m_ImageIndex]->GetTextureVkImageView(),
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .resolveImageView = 0,
                    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
                };
                colorAttachments.emplace_back(attachmentInfo);
            }
            renderInfo.pColorAttachments = colorAttachments.data();
        }
        else
        {
            renderInfo.pColorAttachments = nullptr;
        }

        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdBeginRendering(commandBuffer, &renderInfo);
        }
        else
        {
            m_VkContext.VkCmdBeginRenderingKHR(commandBuffer, &renderInfo);
        }
    }

    void VulkanRendererBackend::VulkanEndRendering()
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        if (m_VkDeviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdEndRendering(commandBuffer);
        }
        else
        {
            m_VkContext.VkCmdEndRenderingKHR(commandBuffer);
        }
    }

    // NOTE: may need to separate to set viewport and set scissor
    void VulkanRendererBackend::SetActiveViewport(glm::vec2 _Start, glm::vec2 _Size)
    {
        VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer();

        VkViewport viewport = {
            .x = _Start.x,
            .y = _Start.y + _Size.y,
            .width = _Size.x,
            .height = -_Size.y,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        // VkViewport viewport = {
        //     .x = _Start.x,
        //     .y = _Start.y,
        //     .width = _Size.x,
        //     .height = _Size.y,
        //     .minDepth = 0.0f,
        //     .maxDepth = 1.0f,
        // };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = {     .x = static_cast<int32_t>(_Start.x),      .y = static_cast<int32_t>(_Start.y) },
            .extent = { .width = static_cast<uint32_t>(_Size.x), .height = static_cast<uint32_t>(_Size.y) },
        };

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VulkanRendererBackend::CreateImGuiDescriptorPool()
    {
        VkDescriptorPoolSize poolSizes[] = {
            {                VK_DESCRIPTOR_TYPE_SAMPLER, 768 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 768 },
            {          VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 768 },
            {          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 768 },
            {   VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 768 },
            {   VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 768 },
            {         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 768 },
            {         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 768 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 768 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 768 },
            {       VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 768 },
        };
        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 768,
            .poolSizeCount = static_cast<uint32_t>(std::size(poolSizes)),
            .pPoolSizes = poolSizes,
        };
        VK_CHECK(vkCreateDescriptorPool(m_VkDeviceWrapper.GetLogicalDevice(), &poolInfo, m_VkContext.VkAllocator,
                                        &m_ImGuiDescriptorPool));
    }

    void VulkanRendererBackend::DestroyImGuiDescriptorPool()
    {
        if (m_ImGuiDescriptorPool)
        {
            vkDestroyDescriptorPool(m_VkDeviceWrapper.GetLogicalDevice(), m_ImGuiDescriptorPool,
                                    m_VkContext.VkAllocator);
        }
    }

    Ref<ImGuiImpl> VulkanRendererBackend::CreateImGuiImpl() { return CreateRef<VulkanImGuiImpl>(); }

    Ref<Shader> VulkanRendererBackend::CreateShader(const ShaderConfig& _ShaderConfig,
                                                    const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs)
    {
        Ref<VulkanShader> shader = CreateRef<VulkanShader>();
        shader->Create(_ShaderConfig, _ShaderStageConfigs);
        shader->Initialize();

        return shader;
    }

    VkCommandBuffer VulkanRendererBackend::CreateAndBeginSingleUseCommandBuffer()
    {
        VkCommandBufferAllocateInfo commandBufferAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_VkDeviceWrapper.GetGraphicsCommandPool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer singleUseCommandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(m_VkDeviceWrapper.GetLogicalDevice(), &commandBufferAllocInfo,
                                          &singleUseCommandBuffer));

        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        VK_CHECK(vkBeginCommandBuffer(singleUseCommandBuffer, &beginInfo));

        return singleUseCommandBuffer;
    }

    void VulkanRendererBackend::DestroyAndEndSingleUseCommandBuffer(VkCommandBuffer _CommandBuffer, VkQueue _Queue,
                                                                    VkCommandPool _CommandPool)
    {
        VK_CHECK(vkEndCommandBuffer(_CommandBuffer));

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &_CommandBuffer,
        };

        VK_CHECK(vkQueueSubmit(_Queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkQueueWaitIdle(_Queue));

        vkFreeCommandBuffers(m_VkDeviceWrapper.GetLogicalDevice(), _CommandPool, 1, &_CommandBuffer);
    }

    Ref<Texture> VulkanRendererBackend::CreateTexture(std::string_view _Name, const TextureProps& _Props)
    {
        Ref<VulkanTexture> texture = CreateRef<VulkanTexture>();
        texture->Create(_Name, _Props);

        return texture;
    }

    Ref<Texture> VulkanRendererBackend::CreateTexture(std::string_view _Name, TextureProps _Props, uint8_t* _Data)
    {
        Ref<VulkanTexture> texture = CreateRef<VulkanTexture>();
        texture->Create(_Name, _Props, _Data);

        return texture;
    }

    Ref<FrameBuffer> VulkanRendererBackend::CreateFrameBuffer(const FrameBufferProps& _Props)
    {
        Ref<VulkanFrameBuffer> frameBuffer = CreateRef<VulkanFrameBuffer>(_Props);

        return frameBuffer;
    }

    Ref<RenderBuffer> VulkanRendererBackend::CreateRenderBuffer(const RenderBufferProps& _Props)
    {
        Ref<VulkanRenderBuffer> renderBuffer = CreateRef<VulkanRenderBuffer>(_Props);

        return renderBuffer;
    }

    Ref<VulkanTexture> VulkanRendererBackend::GetCurrentColorTexture() const
    {
        return m_VkSwapchain.GetVulkanColorTextures()[m_ImageIndex];
    }

    void VulkanRendererBackend::CommandBufferReset(VkCommandBuffer _VkCommandBuffer)
    {
        VK_CHECK(vkResetCommandBuffer(_VkCommandBuffer, 0));
        // command_buffer->state = COMMAND_BUFFER_STATE_READY;
    }

    void VulkanRendererBackend::CommandBufferBegin(VkCommandBuffer _VkCommandBuffer, bool _IsSingleUse,
                                                   bool _IsRenderpassContinue, bool _IsSimultaneousUse)
    {
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
        };

        if (_IsSingleUse)
        {
            beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        }
        if (_IsRenderpassContinue)
        {
            beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        }
        if (_IsSimultaneousUse)
        {
            beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        }

        VK_CHECK(vkBeginCommandBuffer(_VkCommandBuffer, &beginInfo));
        // command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
    }

    void VulkanRendererBackend::CommandBufferEnd(VkCommandBuffer _VkCommandBuffer)
    {
        VK_CHECK(vkEndCommandBuffer(_VkCommandBuffer));
        // command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
    }

    void VulkanRendererBackend::CommandBufferUpdateSubmited(VkCommandBuffer _VkCommandBuffer)
    {
        (void)_VkCommandBuffer;
        // command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
    }

    void VulkanRendererBackend::VerifyRequiredExtensions(const std::vector<const char*>& _RequiredExtensions)
    {
        uint32_t availableExtensionCount = 0;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr));
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data()));

        VEGA_CORE_TRACE("Vulkan available extensions:");
        for (const VkExtensionProperties& availableExtension : availableExtensions)
        {
            VEGA_CORE_TRACE("\t {}", availableExtension.extensionName);
        }

        for (const char* requiredExtension : _RequiredExtensions)
        {
            bool found = false;
            for (const VkExtensionProperties& availableExtension : availableExtensions)
            {
                if (std::strcmp(requiredExtension, availableExtension.extensionName) == 0)
                {
                    found = true;
                    VEGA_CORE_TRACE("Found required extension: {}", requiredExtension);
                    break;
                }
            }

            if (!found)
            {
                VEGA_CORE_ERROR("Required extension {} not found", requiredExtension);
                VEGA_CORE_ASSERT(false, "VerifyRequiredExtensions failed");
            }
        }
    }

    void VulkanRendererBackend::VerifyValidationLayers(const std::vector<const char*>& _RequiredValidationLayers)
    {
        uint32_t availableLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(availableLayerCount);
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

        for (const char* requiredLayer : _RequiredValidationLayers)
        {
            bool found = false;
            for (const VkLayerProperties& availableLayer : availableLayers)
            {
                if (std::strcmp(requiredLayer, availableLayer.layerName) == 0)
                {
                    found = true;
                    VEGA_CORE_TRACE("Found required validation layer: {}", requiredLayer);
                    break;
                }
            }

            if (!found)
            {
                VEGA_CORE_ERROR("Required validation layer {} not found", requiredLayer);
                VEGA_CORE_ASSERT(false, "VerifyValidationLayers failed");
            }
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                       VkDebugUtilsMessageTypeFlagsEXT message_types,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                       void* _UserData)
    {
        switch (message_severity)
        {
            default:
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: VEGA_CORE_ASSERT(false, callback_data->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: VEGA_CORE_WARN(callback_data->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: VEGA_CORE_INFO(callback_data->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: VEGA_CORE_TRACE(callback_data->pMessage); break;
        }
        return VK_FALSE;
    }

}    // namespace Vega
