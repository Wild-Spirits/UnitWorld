#pragma once

#include "Platform/Desktop/Core/GLFWTitleBar.hpp"

namespace Vega
{

    // The content view covers the whole window and the system title bar is transparent. The native window buttons
    // (traffic lights) are kept, as macOS users expect them; the client reserves space for them. Resizing is native.
    class MacOSTitleBar : public GLFWTitleBar
    {
    public:
        MacOSTitleBar(GLFWwindow* _Window);
        virtual ~MacOSTitleBar();

        virtual void OnUpdate() override;
        virtual void OnMouseButton(int _Button, int _Action) override;

        virtual bool IsHasNativeButtons() const override { return true; }
        virtual float GetNativeButtonsWidth() const override;

    protected:
        virtual TitleBarResizeBorderFlags GetClientResizeBorders() const override
        {
            return TitleBarResizeBorderFlagBits::kNone;
        }

    protected:
        // The window drag consumes the mouse up event and it never comes to GLFW
        bool m_IsWaitingButtonRelease = false;
    };

}    // namespace Vega
