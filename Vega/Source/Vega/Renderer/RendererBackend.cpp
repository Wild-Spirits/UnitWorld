#include "RendererBackend.hpp"

#include "Vega/Core/Assert.hpp"
#include "Vega/Plugins/PluginLibrary.hpp"

#include "Platform/OpenGL/Renderer/OpenGlRendererBackend.hpp"

namespace Vega
{

    typedef void (*CreateBackendFunc)(Vega::Ref<Vega::RendererBackend>* _RendererBackend);

    RendererBackend::CreateReturnValue RendererBackend::Create(RendererBackendApi _RendererAPI)
    {
        s_API = _RendererAPI;

        switch (s_API)
        {
            case RendererBackendApi::kVulkan: return CreateVulkanRendererBackend();
            case RendererBackendApi::kDirectX: return CreateReturnValue { .Plugin = nullptr, .Backend = nullptr };
            case RendererBackendApi::kOpenGL: return CreateOpenGlRendererBackend();
        }

        VEGA_CORE_ASSERT(false, "RendererBackend::Create: Unknown RendererAPI!")
        return CreateReturnValue { .Plugin = nullptr, .Backend = nullptr };
    }

    RendererBackend::CreateReturnValue RendererBackend::CreateVulkanRendererBackend()
    {
        PluginLibraryProps pluginLibraryProps {
            .PluginName = "VegaVulkanRenderer",
            .PluginPath = PluginLibrary::GetDefaultPluginPath() / std::filesystem::path("VegaVulkanRenderer"),
            .IsNeedSetPluginGlobals = true,
        };

        Ref<PluginLibrary> pluginLibrary = CreateRef<PluginLibrary>(pluginLibraryProps);
        pluginLibrary->LoadPlugin();

        CreateBackendFunc createBackendFunc = pluginLibrary->LoadFunction<CreateBackendFunc>("CreateRendererBackend");

        Ref<RendererBackend> rendererBackend;
        createBackendFunc(&rendererBackend);

        return CreateReturnValue { .Plugin = pluginLibrary, .Backend = rendererBackend };
    }

    RendererBackend::CreateReturnValue RendererBackend::CreateOpenGlRendererBackend()
    {
        return CreateReturnValue { .Plugin = nullptr, .Backend = CreateRef<OpenGlRendererBackend>() };
    }

}    // namespace Vega
