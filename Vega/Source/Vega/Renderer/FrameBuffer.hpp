#pragma once

#include "Texture.hpp"
#include <string>
#include <vector>

namespace Vega
{

    struct FrameBufferProps
    {
        std::string Name;
        bool IsUsedInFlight;
        bool IsUsedForGui;
        // TODO: Other (Like texture props for creating depth buffer)
    };

    class FrameBuffer
    {
    public:
        virtual ~FrameBuffer() = default;

        virtual const std::vector<Ref<Texture>>& GetTextures() const = 0;
        virtual void* GetInGuiRenderId() const = 0;

        virtual void TransitToGui() = 0;

        virtual void Bind() = 0;

    protected:
    };

}    // namespace Vega
