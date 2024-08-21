#pragma once

#include "Vega/Renderer/RendererBackend.hpp"
#include "VkBase.hpp"
#include "VkDeviceWrapper.hpp"

#include <vector>

namespace LM
{

    class VkRendererBackend : public RendererBackend
    {
    public:
        virtual ~VkRendererBackend() = default;

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;

    private:
        static void VerifyRequiredExtensions(const std::vector<const char*>& _RequiredExtensions);

        static void VerifyValidationLayers(const std::vector<const char*>& _RequiredValidationLayers);

    protected:
        VkInstance m_VkInstance;
        VkAllocationCallbacks* m_VkAllocator = nullptr;
        VkSurfaceKHR m_VkSurface;

        VkDeviceWrapper m_VkDeviceWrapper;

#if defined(_DEBUG)
        VkDebugUtilsMessengerEXT m_VkDebugMessenger;
        PFN_vkSetDebugUtilsObjectNameEXT m_PfnSetDebugUtilsObjectNameEXT;
        PFN_vkSetDebugUtilsObjectTagEXT m_PfnSetDebugUtilsObjectTagEXT;
        PFN_vkCmdBeginDebugUtilsLabelEXT m_PfnCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT m_PfnCmdEndDebugUtilsLabelEXT;
#endif
    };

}    // namespace LM
