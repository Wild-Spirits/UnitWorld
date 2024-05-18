#include "RendererBackend.hpp"

#include "Vega/Core/Assert.hpp"

namespace LM
{

    Ref<RendererBackend> RendererBackend::Create(RendererAPI _RendererBackendType) 
    { 
        s_RendererBackendType = _RendererBackendType;

        switch (s_RendererBackendType)
        {
            case RendererAPI::kVulkan: break;
            case RendererAPI::kDirectX: return nullptr;
            case RendererAPI::kOpenGl: return nullptr;
        }

        LM_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }

}
