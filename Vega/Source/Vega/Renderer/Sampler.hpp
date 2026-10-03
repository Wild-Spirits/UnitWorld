#pragma once

#include "Vega/Core/Base.hpp"

namespace Vega
{

    struct SamplerProps
    {
        enum SamplerFlagBits : uint32_t
        {
            kNone = 0,

            kSRGB = 0,
            kNoSRGB = BIT(0),

            kNoAlpha = 0,
            kAlpha = BIT(1),

            kMagLinear = 0,
            kMagNearest = BIT(2),

            kRepeatS = 0,
            kMirroredRepeatS = BIT(3),
            kClampToEdgeS = BIT(4),
            kClampToBorderS = BIT(5),

            kRepeatT = 0,
            kMirroredRepeatT = BIT(6),
            kClampToEdgeT = BIT(7),
            kClampToBorderT = BIT(8),

            kRepeatR = 0,
            kMirroredRepeatR = BIT(9),
            kClampToEdgeR = BIT(10),
            kClampToBorderR = BIT(11),

            kRepeat = kRepeatS | kRepeatT | kRepeatR,
            kMirroredRepeat = kMirroredRepeatS | kMirroredRepeatT | kMirroredRepeatR,
            kClampToEdge = kClampToEdgeS | kClampToEdgeT | kClampToEdgeR,
            kClampToBorder = kClampToBorderS | kClampToBorderT | kClampToBorderR,

            kMinLinear = 0,
            kMinNearest = BIT(12),
            kMinLinearMipmapLinear = BIT(13),
            kMinLinearMipmapNearest = BIT(14),
            kMinNearestMipmapLinear = BIT(15),
            kMinNearestMipmapNearest = BIT(16),

            kColorAttachment = 0,
            kDepthAttachment = BIT(17),
        };

        typedef uint32_t SamplerFlags;

        uint32_t MipLevels = 1;
        uint32_t ArraySize = 1;
        SamplerFlags Flags = SamplerFlagBits::kNone;
    };

    class Sampler
    {
    public:
        Sampler() = default;
        virtual ~Sampler() = default;

        virtual void OnDetach() = 0;
    };

}    // namespace Vega
