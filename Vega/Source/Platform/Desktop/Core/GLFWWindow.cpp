#include "GLFWWindow.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/Events/KeyEvent.hpp"
#include "Vega/Events/MouseEvent.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/Utils/Log.hpp"

#include <GLFW/glfw3.h>

#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include "GLFW/glfw3native.h"

    #define UNICODE
    #define _UNICODE
    #include <Windows.h>
    #include <Windowsx.h>

#endif

namespace Vega
{

#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)

    bool IsWindowMaximizedAlt(HWND hWnd)
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        if (GetWindowPlacement(hWnd, &wp))
        {
            return wp.showCmd == SW_MAXIMIZE;
        }
        return false;
    }

    glm::ivec4 calcOffset(GLFWWindow::WindowData& _WindowData, HWND hWnd)
    {

        if (IsZoomed(hWnd) != 0)
        {
            LONG offset = std::lround(8.0f * _WindowData.MonitorScale);
            return { offset, offset, offset, offset };
        }
        else
        {
            LONG offset = std::lround(1.0f * _WindowData.MonitorScale);
            return { 1, offset, offset, offset };
        }
    }

    void DrawBorder(GLFWWindow::WindowData& _WindowData, HWND hWnd)
    {
        HDC hdc = GetWindowDC(hWnd);
        RECT rect;
        GetWindowRect(hWnd, &rect);
        OffsetRect(&rect, -rect.left, -rect.top);

        HPEN hPen = CreatePen(PS_SOLID, std::lround(2.0f * _WindowData.MonitorScale), RGB(64, 64, 64));
        HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(hPen);

        ReleaseDC(hWnd, hdc);
    }

    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        // if (uMsg != 124 && uMsg != 125)
        // {
        //     LOG_CORE_WARN("WindowProc {}", uMsg);
        // }

        GLFWWindow::WindowData& data =
            *reinterpret_cast<GLFWWindow::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        switch (uMsg)
        {
            case WM_NCCALCSIZE: {
                if (wParam == TRUE && lParam != NULL)
                {
                    NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                    glm::ivec4 offset = calcOffset(data, hWnd);
                    pParams->rgrc[0].top += offset[0];
                    pParams->rgrc[0].right -= offset[1];
                    pParams->rgrc[0].bottom -= offset[2];
                    pParams->rgrc[0].left += offset[3];
                }
                return 0;
            }
            case WM_NCACTIVATE: return 1;
            // case WM_ACTIVATE: {
            //     LRESULT res = DefWindowProc(hWnd, uMsg, wParam, lParam);
            //     // LONG res = CallWindowProc(original_proc, hWnd, uMsg, wParam, lParam);
            //     DrawBorder(hWnd);
            //     return res;
            // }
            case WM_NCPAINT: {
                DrawBorder(data, hWnd);
                return 0;
            }
            case WM_PAINT: {

                PAINTSTRUCT ps;
                BeginPaint(hWnd, &ps);
                EndPaint(hWnd, nullptr);

                DrawBorder(data, hWnd);
                return 0;
            }
            case WM_NCHITTEST: {
                const int borderWidth = 8;

                POINT mousePos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

                RECT windowRect;
                GetWindowRect(hWnd, &windowRect);

                if (mousePos.y >= windowRect.bottom - borderWidth)
                {
                    if (mousePos.x <= windowRect.left + borderWidth)
                    {
                        return HTBOTTOMLEFT;
                    }
                    else if (mousePos.x >= windowRect.right - borderWidth)
                    {
                        return HTBOTTOMRIGHT;
                    }
                    else
                    {
                        return HTBOTTOM;
                    }
                }
                else if (mousePos.y <= windowRect.top + borderWidth)
                {
                    if (mousePos.x <= windowRect.left + borderWidth)
                    {
                        return HTTOPLEFT;
                    }
                    else if (mousePos.x >= windowRect.right - borderWidth)
                    {
                        return HTTOPRIGHT;
                    }
                    else
                    {
                        return HTTOP;
                    }
                }
                else if (mousePos.x <= windowRect.left + borderWidth)
                {
                    return HTLEFT;
                }
                else if (mousePos.x >= windowRect.right - borderWidth)
                {
                    return HTRIGHT;
                }

                ScreenToClient(hWnd, &mousePos);
                if (mousePos.y < Application::Get().GetMainMenuFrameHeight() &&
                    !Application::Get().GetIsMainMenuAnyItemHovered())
                {
                    return HTCAPTION;
                }

                return HTCLIENT;
            }
        }

        // return DefWindowProc(hWnd, uMsg, wParam, lParam);
        return CallWindowProc(reinterpret_cast<WNDPROC>(data.OriginalProc), hWnd, uMsg, wParam, lParam);
    }

    void disableTitlebar(GLFWwindow* _Window)
    {
        HWND hWnd = glfwGetWin32Window(_Window);
        GLFWWindow::WindowData& data = *reinterpret_cast<GLFWWindow::WindowData*>(glfwGetWindowUserPointer(_Window));
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&data));

        LONG_PTR lStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
        lStyle |= WS_THICKFRAME;
        // lStyle &= ~WS_CAPTION;
        // lStyle &= ~WS_BORDER;
        // lStyle &= ~WS_DLGFRAME;
        SetWindowLongPtr(hWnd, GWL_STYLE, lStyle);
        // MARGINS margins = {0, 0, 0, 0};
        // DwmExtendFrameIntoClientArea(hWnd, &margins);

        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);
        int width = windowRect.right - windowRect.left;
        int height = windowRect.bottom - windowRect.top;

        data.OriginalProc = reinterpret_cast<void*>(GetWindowLongPtr(hWnd, GWLP_WNDPROC));
        data.OriginalProc =
            reinterpret_cast<void*>(SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE);
    }

#endif

    bool GlfwGetWindowMonitor(GLFWmonitor** _Monitor, GLFWwindow* _Window);

    GLFWWindow::GLFWWindow(const WindowProps& _Props) : m_Data(_Props) { Init(); }

    GLFWWindow::~GLFWWindow() { glfwDestroyWindow(m_Window); }

    glm::dvec2 GLFWWindow::GetCursorInWindowPosition() const
    {
        double mouseX, mouseY;
        glfwGetCursorPos(m_Window, &mouseX, &mouseY);

        return { mouseX, mouseY };
    }

    bool GLFWWindow::IsWindowMaximized() const
    {
        int maximized = glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
        return maximized == GLFW_TRUE;
    }

    void GLFWWindow::Maximize() { glfwMaximizeWindow(m_Window); }

    void GLFWWindow::Minimize() { glfwIconifyWindow(m_Window); }

    void GLFWWindow::Restore() { glfwRestoreWindow(m_Window); }

    void GLFWWindow::OnUpdate()
    {
        glfwMakeContextCurrent(m_Window);
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    bool GLFWWindow::Init()
    {
        if (!glfwInit())
        {
            VEGA_CORE_CRITICAL("Failed to initialize GLFW!");
            return false;
        }

        switch (m_Data.RendererAPI)
        {
            case RendererBackendApi::kVulkan: glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); break;
#ifdef _DEBUG
            case RendererBackendApi::kOpenGL: glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); break;
#endif
        }

        m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), NULL, NULL);

        if (!m_Window)
        {
            VEGA_CORE_CRITICAL("Failed to create window!");
            return false;
        }

        GLFWmonitor* nowMonitor = NULL;
        if (!GlfwGetWindowMonitor(&nowMonitor, m_Window))
        {
            nowMonitor = glfwGetPrimaryMonitor();
        }
        glfwGetMonitorContentScale(nowMonitor, NULL, &m_Data.MonitorScale);
        glfwSetWindowUserPointer(m_Window, &m_Data);
        disableTitlebar(m_Window);
        VEGA_CORE_TRACE("Current monitor: {}", static_cast<void*>(nowMonitor));
        VEGA_CORE_TRACE("Current monitor scale: {}", m_Data.MonitorScale);

        glfwMakeContextCurrent(m_Window);

        int wWidth, wHeight;

        glfwGetWindowSize(m_Window, &wWidth, &wHeight);

        m_Data.Width = static_cast<uint32_t>(wWidth);
        m_Data.Height = static_cast<uint32_t>(wHeight);

        VEGA_CORE_INFO("{} {}; {} {}", m_Data.Width, m_Data.Height, wWidth, wHeight);

        SetCallbacks();
        // glfwSetErrorCallback(glfw_error_callback);
        // glfwSetFramebufferSizeCallback(m_Window, window_resize);
        // glfwSetWindowSizeCallback(m_Window, window_resize);
        // glfwSetKeyCallback(m_Window, key_callback);
        // glfwSetMouseButtonCallback(m_Window, mouse_button_callback);
        // glfwSetCursorPosCallback(m_Window, cursor_position_callback);
        // glfwSetScrollCallback(m_Window, scroll_callback);
        // glfwSetCharCallback(m_Window, character_input);
        // glfwSetDropCallback(m_Window, dropping_paths);

        // glfwSetErrorCallback
        // glfwSetFramebufferSizeCallback
        // glfwSetDropCallback

        glfwSwapInterval(0);

        return true;
    }

    void GLFWWindow::SetCallbacks()
    {
        glfwSetWindowPosCallback(m_Window, [](GLFWwindow* _Window, int _PosX, int _PosY) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            GLFWmonitor* nowMonitor = NULL;
            if (!GlfwGetWindowMonitor(&nowMonitor, _Window))
            {
                nowMonitor = glfwGetPrimaryMonitor();
            }
            float newScale = 1.0f;
            glfwGetMonitorContentScale(nowMonitor, NULL, &newScale);

            if (newScale != data.MonitorScale)
            {
                data.MonitorScale = newScale;
                Scope<WindowMonitorScaleChangedEvent> event = CreateScope<WindowMonitorScaleChangedEvent>(newScale);
                data.EventManager->QueueEvent(std::move(event));
            }
        });

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* _Window, int _Width, int _Height) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));
            data.Width = _Width;
            data.Height = _Height;

            {
                Scope<WindowResizeEvent> event = CreateScope<WindowResizeEvent>(_Width, _Height);
                data.EventManager->QueueEvent(std::move(event));
            }

            {
                GLFWmonitor* nowMonitor = NULL;
                if (!GlfwGetWindowMonitor(&nowMonitor, _Window))
                {
                    nowMonitor = glfwGetPrimaryMonitor();
                }
                float newScale = 1.0f;
                glfwGetMonitorContentScale(nowMonitor, NULL, &newScale);

                if (newScale != data.MonitorScale)
                {
                    data.MonitorScale = newScale;
                    Scope<WindowMonitorScaleChangedEvent> event = CreateScope<WindowMonitorScaleChangedEvent>(newScale);
                    data.EventManager->QueueEvent(std::move(event));
                }
            }
        });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* _Window) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            Scope<WindowCloseEvent> event = CreateScope<WindowCloseEvent>();

            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* _Window, int _Key, int _ScanCode, int _Action, int _Mods) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            switch (_Action)
            {
                case GLFW_PRESS: {
                    Scope<KeyPressedEvent> event = CreateScope<KeyPressedEvent>(_Key, 0);
                    data.EventManager->QueueEvent(std::move(event));
                    break;
                }
                case GLFW_RELEASE: {
                    Scope<KeyReleasedEvent> event = CreateScope<KeyReleasedEvent>(_Key);
                    data.EventManager->QueueEvent(std::move(event));
                    break;
                }
                case GLFW_REPEAT: {
                    Scope<KeyPressedEvent> event = CreateScope<KeyPressedEvent>(_Key, 1);
                    data.EventManager->QueueEvent(std::move(event));
                    break;
                }
            }
        });

        glfwSetCharCallback(m_Window, [](GLFWwindow* _Window, unsigned int _Keycode) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            Scope<KeyTypedEvent> event = CreateScope<KeyTypedEvent>(_Keycode);
            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* _Window, int _Button, int _Action, int _Mods) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            switch (_Action)
            {
                case GLFW_PRESS: {
                    Scope<MouseButtonPressedEvent> event = CreateScope<MouseButtonPressedEvent>(_Button);
                    data.EventManager->QueueEvent(std::move(event));
                    break;
                }
                case GLFW_RELEASE: {
                    Scope<MouseButtonReleasedEvent> event = CreateScope<MouseButtonReleasedEvent>(_Button);
                    data.EventManager->QueueEvent(std::move(event));
                    break;
                }
            }
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* _Window, double _OffsetX, double _OffsetY) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            Scope<MouseScrolledEvent> event =
                CreateScope<MouseScrolledEvent>(static_cast<float>(_OffsetX), static_cast<float>(_OffsetY));
            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* _Window, double _PosX, double _PosY) {
            WindowData& data = *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(_Window));

            Scope<MouseMovedEvent> event =
                CreateScope<MouseMovedEvent>(static_cast<float>(_PosX), static_cast<float>(_PosY));
            data.EventManager->QueueEvent(std::move(event));
        });
    }

    bool GlfwGetWindowMonitor(GLFWmonitor** _Monitor, GLFWwindow* _Window)
    {
        bool success = false;

        int windowRectangle[4] = { 0 };
        glfwGetWindowPos(_Window, &windowRectangle[0], &windowRectangle[1]);
        glfwGetWindowSize(_Window, &windowRectangle[2], &windowRectangle[3]);

        int monitorsSize = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorsSize);

        GLFWmonitor* closestMonitor = NULL;
        int maxOverlapArea = 0;

        for (int i = 0; i < monitorsSize; ++i)
        {
            int monitorPosition[2] = { 0 };
            glfwGetMonitorPos(monitors[i], &monitorPosition[0], &monitorPosition[1]);

            const GLFWvidmode* monitorVideoMode = glfwGetVideoMode(monitors[i]);

            int monitorRectangle[4] = {
                monitorPosition[0],
                monitorPosition[1],
                monitorVideoMode->width,
                monitorVideoMode->height,
            };

            if (!(((windowRectangle[0] + windowRectangle[2]) < monitorRectangle[0]) ||
                  (windowRectangle[0] > (monitorRectangle[0] + monitorRectangle[2])) ||
                  ((windowRectangle[1] + windowRectangle[3]) < monitorRectangle[1]) ||
                  (windowRectangle[1] > (monitorRectangle[1] + monitorRectangle[3]))))
            {
                int intersectionRectangle[4] = { 0 };

                // x, width
                if (windowRectangle[0] < monitorRectangle[0])
                {
                    intersectionRectangle[0] = monitorRectangle[0];

                    if ((windowRectangle[0] + windowRectangle[2]) < (monitorRectangle[0] + monitorRectangle[2]))
                    {
                        intersectionRectangle[2] = (windowRectangle[0] + windowRectangle[2]) - intersectionRectangle[0];
                    }
                    else
                    {
                        intersectionRectangle[2] = monitorRectangle[2];
                    }
                }
                else
                {
                    intersectionRectangle[0] = windowRectangle[0];

                    if ((monitorRectangle[0] + monitorRectangle[2]) < (windowRectangle[0] + windowRectangle[2]))
                    {
                        intersectionRectangle[2] =
                            (monitorRectangle[0] + monitorRectangle[2]) - intersectionRectangle[0];
                    }
                    else
                    {
                        intersectionRectangle[2] = windowRectangle[2];
                    }
                }

                // y, height
                if (windowRectangle[1] < monitorRectangle[1])
                {
                    intersectionRectangle[1] = monitorRectangle[1];

                    if ((windowRectangle[1] + windowRectangle[3]) < (monitorRectangle[1] + monitorRectangle[3]))
                    {
                        intersectionRectangle[3] = (windowRectangle[1] + windowRectangle[3]) - intersectionRectangle[1];
                    }
                    else
                    {
                        intersectionRectangle[3] = monitorRectangle[3];
                    }
                }
                else
                {
                    intersectionRectangle[1] = windowRectangle[1];

                    if ((monitorRectangle[1] + monitorRectangle[3]) < (windowRectangle[1] + windowRectangle[3]))
                    {
                        intersectionRectangle[3] =
                            (monitorRectangle[1] + monitorRectangle[3]) - intersectionRectangle[1];
                    }
                    else
                    {
                        intersectionRectangle[3] = windowRectangle[3];
                    }
                }

                // int overlapArea = intersectionRectangle[3] * intersectionRectangle[4];
                int overlapArea = intersectionRectangle[2] * intersectionRectangle[3];

                if (overlapArea > maxOverlapArea)
                {
                    closestMonitor = monitors[i];
                    maxOverlapArea = overlapArea;
                }
            }
        }

        if (closestMonitor)
        {
            *_Monitor = closestMonitor;
            success = true;
        }

        // true: monitor contains the monitor the window is most on
        // false: monitor is unmodified
        return success;
    }

}    // namespace Vega
