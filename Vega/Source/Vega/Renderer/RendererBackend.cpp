#include "RendererBackend.hpp"

#include "Platform/OpenGL/Renderer/OpenGlRendererBackend.hpp"
#include "Vega/Core/Assert.hpp"
#include "Vega/Plugins/PluginLibrary.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <format>

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

    Ref<Texture> RendererBackend::CreateTexture(std::string_view _Name, const std::filesystem::path& _Filename,
                                                const TextureProps& _Props)
    {
        int width, height, channels;
        // stbi_set_flip_vertically_on_load(1);
        unsigned char* data = stbi_load(_Filename.string().c_str(), &width, &height, &channels, 0);
        VEGA_CORE_ASSERT(data, std::format("Failed to load texture from file: {}", _Filename.string()));

        TextureProps props = _Props;
        props.Width = width;
        props.Height = height;
        props.ChannelCount = channels;

        Ref<Texture> texture = CreateTexture(_Name, props, data);
        stbi_image_free(data);

        return texture;
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
