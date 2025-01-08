#pragma once

#include "Vega/Renderer/RendererBackend.hpp"
#include "VkBase.hpp"
#include "VkDeviceWrapper.hpp"
#include "VkSwapchain.hpp"

#include <vector>

namespace Vega
{

    class VkRendererBackend : public RendererBackend
    {
    public:
        virtual ~VkRendererBackend() = default;

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual bool OnWindowCreate(/* Ref<Wiondow> _Window */) override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;

        inline const VkContext& GetVkContext() const { return m_VkContext; }
        inline const VkDeviceWrapper& GetVkDeviceWrapper() const { return m_VkDeviceWrapper; }

    private:
        static void VerifyRequiredExtensions(const std::vector<const char*>& _RequiredExtensions);

        static void VerifyValidationLayers(const std::vector<const char*>& _RequiredValidationLayers);

    protected:
        VkSurfaceKHR m_VkSurface;

        VkContext m_VkContext = {};

        std::vector<VkSampler> samplers;

        VkDeviceWrapper m_VkDeviceWrapper;
        VkSwapchain m_VkSwapchain;
    };

}    // namespace Vega
