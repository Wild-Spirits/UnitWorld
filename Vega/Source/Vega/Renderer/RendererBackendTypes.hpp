#pragma once

#include "Vega/Core/Base.hpp"

namespace Vega
{

    namespace RendererBackendConfig
    {

        enum FlagBits : uint16_t
        {
            kRendererConfigFlagVsyncEnabledBit = BIT(0),
            kRendererConfigFlagPowerSavingBit = BIT(1),
            kRendererConfigFlagEnableValidation = BIT(2),
        };

        typedef uint16_t Flags;

    }    // namespace RendererBackendConfig

}    // namespace Vega
