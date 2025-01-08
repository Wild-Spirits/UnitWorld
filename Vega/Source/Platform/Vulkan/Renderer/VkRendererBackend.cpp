#include "VkRendererBackend.hpp"

#include "Vega/Utils/Log.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_DESKTOP
    #include "GLFW/glfw3.h"
    #include "Platform/Desktop/Core/GLFWWindow.hpp"
    #include "Vega/Core/Application.hpp"
#endif

#include "Platform/Vulkan/Platform/VkPlatform.hpp"
#include "Platform/Vulkan/Utils/VkUtils.hpp"

namespace Vega
{

    VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                   VkDebugUtilsMessageTypeFlagsEXT message_types,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                   void* _UserData)
    {
        switch (message_severity)
        {
            default:
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: VEGA_CORE_ERROR(callback_data->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: VEGA_CORE_WARN(callback_data->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: VEGA_CORE_INFO(callback_data->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: VEGA_CORE_TRACE(callback_data->pMessage); break;
        }
        return VK_FALSE;
    }

    bool VkRendererBackend::Init()
    {
        VEGA_CORE_TRACE("Initialize VkRendererBackend");

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
        VkGetPlatformRequiredExtensionNames(&instanceExtensions);
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
        if (!VkResultIsSuccess(instanceResult))
        {
            VEGA_CORE_CRITICAL("Vulkan instance creation failed with result: {}", VkResultString(instanceResult, true));
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
            .pfnUserCallback = VkDebugCallback
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
            VEGA_CORE_CRITICAL("Failed to initialize VkDeviceWrapper");
            return false;
        }

        return OnWindowCreate();
    }

    void VkRendererBackend::Shutdown()
    {
        // TODO: Implement
    }

    bool VkRendererBackend::OnWindowCreate(/* Ref<Window> _Window */)
    {
        auto _Window = Application::Get().GetWindow();

        // TODO: Get window title, id, or address in memory
        VEGA_CORE_TRACE("Creating Vulkan surface for window {}", "Main");
#ifdef VEGA_PLATFORM_DESKTOP
        VkResult surfaceResult =
            glfwCreateWindowSurface(m_VkContext.VkInstance, static_cast<GLFWwindow*>(_Window->GetNativeWindow()),
                                    m_VkContext.VkAllocator, &m_VkSurface);

        if (surfaceResult != VK_SUCCESS)
        {
            VEGA_CORE_CRITICAL("Failed to create platform surface for window {}", "Main");
            return false;
        }
#endif

        VEGA_CORE_TRACE("Vulkan surface created for window {}", "Main");

        m_VkSwapchain.Create(m_VkContext, m_VkDeviceWrapper, m_VkSurface);

        // TODO: Add code from "on window create"

        return true;
    }

    void VkRendererBackend::BeginFrame() { }

    void VkRendererBackend::EndFrame() { }

    void VkRendererBackend::VerifyRequiredExtensions(const std::vector<const char*>& _RequiredExtensions)
    {
        uint32_t availableExtensionCount = 0;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr));
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data()));

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

    void VkRendererBackend::VerifyValidationLayers(const std::vector<const char*>& _RequiredValidationLayers)
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

}    // namespace Vega
