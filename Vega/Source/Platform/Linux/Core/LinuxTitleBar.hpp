#pragma once

#include "Platform/Desktop/Core/GLFWTitleBar.hpp"

#include "glm/vec2.hpp"

#include <array>

struct GLFWcursor;

namespace Vega
{

    // X11 only: the window is undecorated, move and resize are delegated to the window manager by
    // _NET_WM_MOVERESIZE (snapping and other WM features keep working). Wayland doesn't let clients move windows
    // without xdg_toplevel, which GLFW doesn't expose, so the system decorations are used there.
    class LinuxTitleBar : public GLFWTitleBar
    {
    public:
        LinuxTitleBar(GLFWwindow* _Window);
        virtual ~LinuxTitleBar();

        static bool IsSupported();

        virtual void OnUpdate() override;
        virtual void OnMouseButton(int _Button, int _Action) override;

    protected:
        void StartMoveResize(long _Direction);
        void ReleaseStuckMouseButton();
        void UpdateCursor();

    protected:
        enum CursorType
        {
            kCursorNone,
            kCursorResizeEW,
            kCursorResizeNS,
            kCursorResizeNWSE,
            kCursorResizeNESW,
            kCursorCount,
        };

        std::array<GLFWcursor*, kCursorCount> m_Cursors = {};
        CursorType m_CurrentCursor = kCursorNone;

        // The window manager grabs the pointer on move/resize and the button release never comes to GLFW
        bool m_IsWaitingButtonRelease = false;

        // glfwGetTime starts from 0, a negative value guarantees the first click isn't a double click
        double m_LastCaptionClickTime = -1.0;
        glm::vec2 m_LastCaptionClickPos = { 0.0f, 0.0f };
    };

}    // namespace Vega
