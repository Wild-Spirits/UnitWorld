#pragma once

#include "Vega/ImGui/ImGuiImpl.hpp"

namespace Vega
{

    class OGLImGuiImpl : public ImGuiImpl
    {
    public:
        virtual void Init() override;
        virtual void NewFrame() override;
        virtual void RenderFrame() override;
        virtual void Shutdown() override;
        virtual void RecreateFontTexture() override;
    };

}    // namespace Vega
