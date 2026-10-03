#pragma once

#include "Platform/Desktop/Core/GLFWTitleBar.hpp"

namespace Vega
{

    // The system caption is removed by WM_NCCALCSIZE, left/right/bottom borders stay native (invisible resize
    // borders, shadow, snap). The top resize border and the caption are emulated in WM_NCHITTEST.
    class WinTitleBar : public GLFWTitleBar
    {
    public:
        WinTitleBar(GLFWwindow* _Window);
        virtual ~WinTitleBar();

    protected:
        virtual TitleBarResizeBorderFlags GetClientResizeBorders() const override
        {
            return TitleBarResizeBorderFlagBits::kTop;
        }
        virtual float GetResizeBorderWidth() const override;
    };

}    // namespace Vega
