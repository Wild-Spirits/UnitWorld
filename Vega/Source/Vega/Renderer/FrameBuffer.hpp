#pragma once

#include "Texture.hpp"
#include <string>
#include <vector>

namespace Vega
{

    struct FrameBufferProps
    {
        std::string Name;
        uint32_t Width;
        uint32_t Height;
        bool IsUsedInFlight = true;
        // TODO: Other (Like texture props for creating depth buffer)
    };

    class FrameBuffer
    {
    public:
        virtual ~FrameBuffer() = default;

        virtual void Destroy() = 0;
        virtual void Resize(uint32_t _Width, uint32_t _Height) = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void TransitColorAttachmentToGui(size_t _AttachmentIndex = 0) = 0;

        virtual void BindAndClearColorDepthStencil() = 0;

        virtual void Bind() = 0;

    protected:
    };

}    // namespace Vega
