#include "LinuxTitleBar.hpp"

#include "Platform/Platform.hpp"

#if defined(VEGA_PLATFORM_LINUX_DESKTOP)

    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_X11
    #include <GLFW/glfw3native.h>

    #include <cmath>

namespace Vega
{

    namespace
    {

        // _NET_WM_MOVERESIZE directions from the EWMH specification
        constexpr long kNetWmMoveResizeSizeTopLeft = 0;
        constexpr long kNetWmMoveResizeSizeTop = 1;
        constexpr long kNetWmMoveResizeSizeTopRight = 2;
        constexpr long kNetWmMoveResizeSizeRight = 3;
        constexpr long kNetWmMoveResizeSizeBottomRight = 4;
        constexpr long kNetWmMoveResizeSizeBottom = 5;
        constexpr long kNetWmMoveResizeSizeBottomLeft = 6;
        constexpr long kNetWmMoveResizeSizeLeft = 7;
        constexpr long kNetWmMoveResizeMove = 8;
        // Source indication: request from a normal application
        constexpr long kNetWmSourceApplication = 1;

        // X11 has no system double click settings, values are close to the GTK defaults
        constexpr double kDoubleClickTime = 0.4;
        constexpr float kDoubleClickDistance = 5.0f;

        long GetMoveResizeDirection(TitleBarHit _Hit)
        {
            switch (_Hit)
            {
                case TitleBarHit::kResizeLeft: return kNetWmMoveResizeSizeLeft;
                case TitleBarHit::kResizeRight: return kNetWmMoveResizeSizeRight;
                case TitleBarHit::kResizeTop: return kNetWmMoveResizeSizeTop;
                case TitleBarHit::kResizeBottom: return kNetWmMoveResizeSizeBottom;
                case TitleBarHit::kResizeTopLeft: return kNetWmMoveResizeSizeTopLeft;
                case TitleBarHit::kResizeTopRight: return kNetWmMoveResizeSizeTopRight;
                case TitleBarHit::kResizeBottomLeft: return kNetWmMoveResizeSizeBottomLeft;
                case TitleBarHit::kResizeBottomRight: return kNetWmMoveResizeSizeBottomRight;
                default: return kNetWmMoveResizeMove;
            }
        }

    }    // namespace

    LinuxTitleBar::LinuxTitleBar(GLFWwindow* _Window) : GLFWTitleBar(_Window)
    {
        glfwSetWindowAttrib(_Window, GLFW_DECORATED, GLFW_FALSE);

        // Can be nullptr if the cursor theme has no such shape: the default cursor is used then
        m_Cursors[kCursorResizeEW] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
        m_Cursors[kCursorResizeNS] = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
        m_Cursors[kCursorResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
        m_Cursors[kCursorResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    }

    LinuxTitleBar::~LinuxTitleBar()
    {
        if (m_CurrentCursor != kCursorNone)
        {
            glfwSetCursor(m_Window, nullptr);
        }

        for (GLFWcursor* cursor : m_Cursors)
        {
            if (cursor)
            {
                glfwDestroyCursor(cursor);
            }
        }
    }

    bool LinuxTitleBar::IsSupported() { return glfwGetPlatform() == GLFW_PLATFORM_X11; }

    void LinuxTitleBar::OnUpdate()
    {
        ReleaseStuckMouseButton();
        UpdateCursor();
    }

    void LinuxTitleBar::OnMouseButton(int _Button, int _Action)
    {
        if (_Button != GLFW_MOUSE_BUTTON_LEFT || _Action != GLFW_PRESS)
        {
            return;
        }

        glm::vec2 cursorPos = GetCursorPos();
        TitleBarHit hit = HitTest(cursorPos);

        if (IsResizeHit(hit))
        {
            StartMoveResize(GetMoveResizeDirection(hit));
            return;
        }

        if (hit != TitleBarHit::kCaption)
        {
            return;
        }

        double time = glfwGetTime();
        bool isDoubleClick = time - m_LastCaptionClickTime <= kDoubleClickTime &&
                             std::abs(cursorPos.x - m_LastCaptionClickPos.x) <= kDoubleClickDistance &&
                             std::abs(cursorPos.y - m_LastCaptionClickPos.y) <= kDoubleClickDistance;
        // Reset after a double click, so the third click doesn't make one more
        m_LastCaptionClickTime = isDoubleClick ? -1.0 : time;
        m_LastCaptionClickPos = cursorPos;

        if (isDoubleClick)
        {
            if (IsMaximized())
            {
                glfwRestoreWindow(m_Window);
            }
            else
            {
                glfwMaximizeWindow(m_Window);
            }
            return;
        }

        StartMoveResize(kNetWmMoveResizeMove);
    }

    void LinuxTitleBar::StartMoveResize(long _Direction)
    {
        Display* display = glfwGetX11Display();
        ::Window xWindow = glfwGetX11Window(m_Window);

        ::Window root, child;
        int rootX, rootY, windowX, windowY;
        unsigned int mask;
        if (!XQueryPointer(display, xWindow, &root, &child, &rootX, &rootY, &windowX, &windowY, &mask))
        {
            return;
        }

        // The button press made an implicit pointer grab for our window, the window manager can't take the pointer
        // until it's released
        XUngrabPointer(display, CurrentTime);

        XEvent event = {};
        event.xclient.type = ClientMessage;
        event.xclient.window = xWindow;
        event.xclient.message_type = XInternAtom(display, "_NET_WM_MOVERESIZE", False);
        event.xclient.format = 32;
        event.xclient.data.l[0] = rootX;
        event.xclient.data.l[1] = rootY;
        event.xclient.data.l[2] = _Direction;
        event.xclient.data.l[3] = Button1;
        event.xclient.data.l[4] = kNetWmSourceApplication;

        XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(display);

        m_IsWaitingButtonRelease = true;
    }

    void LinuxTitleBar::ReleaseStuckMouseButton()
    {
        if (!m_IsWaitingButtonRelease)
        {
            return;
        }

        Display* display = glfwGetX11Display();
        ::Window xWindow = glfwGetX11Window(m_Window);

        ::Window root, child;
        int rootX, rootY, windowX, windowY;
        unsigned int mask = 0;
        XQueryPointer(display, xWindow, &root, &child, &rootX, &rootY, &windowX, &windowY, &mask);
        if (mask & Button1Mask)
        {
            // Move/resize is still in progress
            return;
        }

        m_IsWaitingButtonRelease = false;

        // The release could still come to GLFW, e.g. if the window manager doesn't support _NET_WM_MOVERESIZE
        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        {
            return;
        }

        XEvent event = {};
        event.xbutton.type = ButtonRelease;
        event.xbutton.display = display;
        event.xbutton.window = xWindow;
        event.xbutton.root = root;
        event.xbutton.subwindow = None;
        event.xbutton.time = CurrentTime;
        event.xbutton.x = windowX;
        event.xbutton.y = windowY;
        event.xbutton.x_root = rootX;
        event.xbutton.y_root = rootY;
        event.xbutton.state = Button1Mask;
        event.xbutton.button = Button1;
        event.xbutton.same_screen = True;

        XSendEvent(display, xWindow, False, ButtonReleaseMask, &event);
        XFlush(display);
    }

    void LinuxTitleBar::UpdateCursor()
    {
        CursorType cursor = kCursorNone;
        switch (HitTest(GetCursorPos()))
        {
            case TitleBarHit::kResizeLeft:
            case TitleBarHit::kResizeRight: cursor = kCursorResizeEW; break;
            case TitleBarHit::kResizeTop:
            case TitleBarHit::kResizeBottom: cursor = kCursorResizeNS; break;
            case TitleBarHit::kResizeTopLeft:
            case TitleBarHit::kResizeBottomRight: cursor = kCursorResizeNWSE; break;
            case TitleBarHit::kResizeTopRight:
            case TitleBarHit::kResizeBottomLeft: cursor = kCursorResizeNESW; break;
            default: break;
        }

        if (cursor == m_CurrentCursor)
        {
            return;
        }

        // On the resize border the client doesn't change the cursor (see Window::IsCursorOnResizeBorder), outside
        // of it the client sets its own cursor again
        m_CurrentCursor = cursor;
        glfwSetCursor(m_Window, m_Cursors[cursor]);
    }

}    // namespace Vega

#endif
