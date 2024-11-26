#include "RendererBackend.hpp"

#include "Vega/Core/Assert.hpp"

#include "Platform/OpenGL/Renderer/OGLRendererBackend.hpp"
#include "Platform/Vulkan/Renderer/VkRendererBackend.hpp"

namespace Vega
{

    Ref<RendererBackend> RendererBackend::Create(API _RendererAPI)
    {
        s_API = _RendererAPI;

        switch (s_API)
        {
            case API::kVulkan: return CreateRef<VkRendererBackend>();
            case API::kDirectX: return nullptr;
            case API::kOpenGL: return CreateRef<OGLRendererBackend>();
        }

        VEGA_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }

}    // namespace Vega
