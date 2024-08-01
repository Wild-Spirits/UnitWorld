#include "VkRendererBackend.hpp"

#include "Vega/Core/Assert.hpp"
#include "Vega/Utils/Log.hpp"

#include "Platform/Platform.hpp"

#ifdef LM_PLATFORM_DESKTOP
    #include "GLFW/glfw3.h"
    #include "Platform/Desktop/Core/GLFWWindow.hpp"
    #include "Vega/Core/Application.hpp"
#endif

#include "Platform/Vulkan/Platform/VkPlatform.hpp"
#include "Platform/Vulkan/Utils/VkUtils.hpp"

namespace LM
{

    bool VkRendererBackend::Init()
    {
        LM_CORE_TRACE("Initialize VkRendererBackend");

        m_VkAllocator = nullptr;

        uint32_t vkApiVersion = 0;
        vkEnumerateInstanceVersion(&vkApiVersion);

        uint32_t vkApiVersionMajor = VK_VERSION_MAJOR(vkApiVersion);
        uint32_t vkApiVersionMinor = VK_VERSION_MINOR(vkApiVersion);
        uint32_t vkApiVersionPatch = VK_VERSION_PATCH(vkApiVersion);
        LM_CORE_TRACE("Vulkan API Version: {}.{}.{}", vkApiVersionMajor, vkApiVersionMinor, vkApiVersionPatch);

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

#ifdef LM_PLATFORM_APPLE
        instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        VkResult instanceResult = vkCreateInstance(&instanceCreateInfo, nullptr, &m_VkInstance);

        if (!VkResultIsSuccess(instanceResult))
        {
            LM_CORE_CRITICAL("Vulkan instance creation failed with result: {}", VkResultString(instanceResult, true));
            return false;
        }

#ifdef LM_PLATFORM_DESKTOP
        VkResult surfaceResult = glfwCreateWindowSurface(
            m_VkInstance, static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow()), m_VkAllocator,
            &m_VkSurface);

        if (surfaceResult != VK_SUCCESS)
        {
            LM_CORE_CRITICAL("Vulkan surface creation failed.");
            return false;
        }
#endif

        LM_CORE_TRACE("Vulkan instance created");
        return true;
    }

    void VkRendererBackend::Shutdown() { }

    void VkRendererBackend::BeginFrame() { }

    void VkRendererBackend::EndFrame() { }

    void VkRendererBackend::VerifyRequiredExtensions(const std::vector<const char*>& _RequiredExtensions)
    {
        uint32_t availableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

        for (const char* requiredExtension : _RequiredExtensions)
        {
            bool found = false;
            for (const VkExtensionProperties& availableExtension : availableExtensions)
            {
                if (std::strcmp(requiredExtension, availableExtension.extensionName) == 0)
                {
                    found = true;
                    LM_CORE_TRACE("Found required extension: {}", requiredExtension);
                    break;
                }
            }

            if (!found)
            {
                LM_CORE_ERROR("Required extension {} not found", requiredExtension);
                LM_CORE_ASSERT(false, "VerifyRequiredExtensions failed");
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
                    LM_CORE_TRACE("Found required validation layer: {}", requiredLayer);
                    break;
                }
            }

            if (!found)
            {
                LM_CORE_ERROR("Required validation layer {} not found", requiredLayer);
                LM_CORE_ASSERT(false, "VerifyValidationLayers failed");
            }
        }
    }

}    // namespace LM
