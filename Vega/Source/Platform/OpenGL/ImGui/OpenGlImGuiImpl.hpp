#pragma once

#include "Vega/ImGui/ImGuiImpl.hpp"

struct GLFWwindow;

namespace Vega
{

    class OpenGlImGuiImpl : public ImGuiImpl
    {
    public:
        virtual void SetImGuiGlobalData(ImGuiContext*, ImGuiMemAllocFunc*, ImGuiMemFreeFunc*, void**) override {};

        virtual void Init() override;
        virtual void NewFrame() override;
        virtual void RenderFrame() override;
        virtual void Shutdown() override;
        virtual void RecreateFontTexture() override;

        virtual void BackupCurrentWindowContext();
        virtual void RestoreCurrentWindowContext();

    protected:
        GLFWwindow* m_CurrentWindowContext;
    };

}    // namespace Vega
