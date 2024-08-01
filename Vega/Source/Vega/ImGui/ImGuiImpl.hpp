#pragma once

#include "Vega/Core/Base.hpp"

namespace LM
{

    class ImGuiImpl
    {
    public:
        virtual void Init() = 0;
        virtual void NewFrame() = 0;
        virtual void RenderFrame() = 0;
        virtual void Shutdown() = 0;

        virtual void RecreateFontTexture() = 0;

        static Scope<ImGuiImpl> Create();

    protected:
    };

}    // namespace LM
