#pragma once

#include "Vega/Core/Base.hpp"

#include "glm/ext/vector_float4.hpp"

#include <string_view>

namespace Vega
{

    struct TextureProps
    {
        enum class TextureType : uint32_t
        {
            k2D = 0,
            kCubeMap,
            k3D,
            kArray,
        };

        enum TextureFlagBits : uint32_t
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

        typedef uint32_t TextureFlags;

        TextureType Type;
        uint32_t Width;
        uint32_t Height;
        uint32_t ChannelCount;
        uint32_t MipLevels = 1;
        uint32_t ArraySize = 1;
        bool IsUsedForGui = false;
        TextureFlags Flags = TextureFlagBits::kNone;
    };

    class Texture
    {
    public:
        Texture() = default;
        virtual ~Texture() = default;

        virtual void Create(std::string_view _Name, const TextureProps& _Props) = 0;
        virtual void Create(std::string_view _Name, const TextureProps& _Props, uint8_t* _Data) = 0;
        virtual void Destroy() = 0;
        virtual void Resize(std::string_view _Name, uint32_t _NewWidth, uint32_t _NewHeight) = 0;

        virtual void ClearColor(const glm::vec4& _ClearColor) = 0;
        virtual void ClearDepthStencil() = 0;

        virtual void* GetTextureGuiId() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetMipLevels() const = 0;
        virtual uint32_t GetArraySize() const = 0;
    };

}    // namespace Vega
