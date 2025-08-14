#pragma once

#include <cstdint>

namespace Vega
{

    enum class RendererBackendApi : uint16_t
    {
        kNone = 0,
        kVulkan,
        kDirectX,
        kOpenGL
    };

}    // namespace Vega
