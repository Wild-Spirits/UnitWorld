#pragma once

#include "Vega/Renderer/RendererBackend.hpp"

#include <vector>

#include <vulkan/vulkan.h>

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
    };

}    // namespace LM
