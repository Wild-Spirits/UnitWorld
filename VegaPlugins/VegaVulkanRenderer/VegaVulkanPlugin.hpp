#pragma once

#include "Vega/Core/Base.hpp"

#include "Renderer/VulkanRendererBackend.hpp"
#include "Vega/Utils/PluginData.hpp"

#ifdef VEGA_VULKAN_RENDERER_EXPORTS
    #ifdef _MSC_VER
        #define VEGA_VULKAN_RENDERER_API __declspec(dllexport)
    #else
        #define VEGA_VULKAN_RENDERER_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _MSC_VER
        #define VEGA_VULKAN_RENDERER_API __declspec(dllimport)
    #else
        #define VEGA_VULKAN_RENDERER_API
    #endif
#endif

extern "C" {
VEGA_VULKAN_RENDERER_API void SetPluginGlobals(Vega::PluginData* _PluginData);

VEGA_VULKAN_RENDERER_API void CreateRendererBackend(Vega::Ref<Vega::RendererBackend>* _RendererBackend);
};
