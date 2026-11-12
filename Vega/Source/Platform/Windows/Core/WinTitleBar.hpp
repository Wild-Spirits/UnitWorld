#pragma once

#include "Platform/Desktop/Core/GLFWTitleBar.hpp"

namespace Vega
{

    // The system caption is removed by WM_NCCALCSIZE, left/right/bottom borders stay native (invisible resize
    // borders, shadow, snap). The top resize border and the caption are emulated in WM_NCHITTEST.
    // In resize-only mode (secondary ImGui viewports) the client covers the whole window rect and all resize
    // borders are emulated: GLFW sizes undecorated windows without a frame, native insets would desync it.
    class WinTitleBar : public GLFWTitleBar
    {
    public:
        WinTitleBar(GLFWwindow* _Window, TitleBarMode _Mode = TitleBarMode::kFull);
        virtual ~WinTitleBar();

        virtual void StartCaptionMove() override;

    protected:
        virtual TitleBarResizeBorderFlags GetClientResizeBorders() const override
        {
            return IsResizeOnly() ? TitleBarResizeBorderFlagBits::kAll : TitleBarResizeBorderFlagBits::kTop;
        }
        virtual float GetResizeBorderWidth() const override;
    };

}    // namespace Vega
