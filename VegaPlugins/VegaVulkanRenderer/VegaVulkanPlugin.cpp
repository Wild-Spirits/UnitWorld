#include "VegaVulkanPlugin.hpp"

void SetPluginGlobals(Vega::PluginData* _PluginData) { return _PluginData->SetPluginData(); }

void CreateRendererBackend(Vega::Ref<Vega::RendererBackend>* _RendererBackend)
{
    *_RendererBackend = Vega::CreateRef<Vega::VulkanRendererBackend>();
}
