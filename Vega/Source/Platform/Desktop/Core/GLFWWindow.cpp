#include "GLFWWindow.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/Events/KeyEvent.hpp"
#include "Vega/Events/MouseEvent.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/Utils/Log.hpp"

#include <GLFW/glfw3.h>

// Native handles for GetNativeWindowInfo; no Vulkan headers are needed here, the renderer plugin creates
// its surfaces from these handles itself
#ifdef VEGA_PLATFORM_WINDOWS
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(VEGA_PLATFORM_LINUX)
    #define GLFW_EXPOSE_NATIVE_WAYLAND
    #define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#ifdef VEGA_PLATFORM_LINUX
    #include <cstdlib>
    #include <cstring>
#endif

namespace Vega
{

#ifdef VEGA_PLATFORM_LINUX
    namespace
    {

        void LogWindowSystem()
        {
            const bool isWaylandBackend = (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND);

            // Session type: XDG_SESSION_TYPE is set by logind, WAYLAND_DISPLAY is a fallback
            // Wayland session indicator for environments without logind
            const char* sessionType = std::getenv("XDG_SESSION_TYPE");
            const bool isWaylandSession = (sessionType != nullptr && std::strcmp(sessionType, "wayland") == 0) ||
                                          (std::getenv("WAYLAND_DISPLAY") != nullptr);

            if (!isWaylandBackend && isWaylandSession)
            {
                VEGA_CORE_INFO("Window system: X11 (XWayland under Wayland session)");
            }
            else
            {
                VEGA_CORE_INFO("Window system: {}", isWaylandBackend ? "Wayland" : "X11");
            }
        }

    }    // namespace
#endif

    bool GlfwGetWindowMonitor(GLFWmonitor** _Monitor, GLFWwindow* _Window);

    GLFWWindow::GLFWWindow(const WindowProps& _Props) : m_Data(_Props) { Init(); }

    GLFWWindow::~GLFWWindow()
    {
        // Restores the window state (e.g. the window procedure on Windows) while the window is still alive
        m_TitleBar.reset();
        m_Data.TitleBar = nullptr;

        glfwDestroyWindow(m_Window);
    }

    glm::dvec2 GLFWWindow::GetCursorInWindowPosition() const
    {
        double mouseX, mouseY;
        glfwGetCursorPos(m_Window, &mouseX, &mouseY);

        return { mouseX, mouseY };
    }

    glm::uvec2 GLFWWindow::GetFramebufferSize() const
    {
        int width, height;
        glfwGetFramebufferSize(m_Window, &width, &height);

        return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

    bool GLFWWindow::IsWindowMaximized() const
    {
        int maximized = glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
        return maximized == GLFW_TRUE;
    }

    NativeWindowInfo GLFWWindow::GetNativeWindowInfo() const
    {
        NativeWindowInfo info;

#ifdef VEGA_PLATFORM_WINDOWS
        info.System = WindowSystem::kWin32;
        info.Window = glfwGetWin32Window(m_Window);
        info.Display = GetModuleHandle(nullptr);
#elif defined(VEGA_PLATFORM_LINUX)
        // GLFW 3.4 picks the window system at runtime, so the handles are queried for the active one
        switch (glfwGetPlatform())
        {
            case GLFW_PLATFORM_WAYLAND:
                info.System = WindowSystem::kWayland;
                info.Window = glfwGetWaylandWindow(m_Window);
                info.Display = glfwGetWaylandDisplay();
                break;
            case GLFW_PLATFORM_X11:
                info.System = WindowSystem::kX11;
                info.Window = reinterpret_cast<void*>(glfwGetX11Window(m_Window));
                info.Display = glfwGetX11Display();
                break;
            default:
                VEGA_CORE_CRITICAL("Unsupported GLFW platform: {}", glfwGetPlatform());
                break;
        }
#endif

        return info;
    }

    // Usually called from the GUI while the frame is being recorded: the state change synchronously sends resize
    // and other events, so it's postponed until OnUpdate
    void GLFWWindow::Maximize() { m_PendingShowCommand = ShowCommand::kMaximize; }

    void GLFWWindow::Minimize() { m_PendingShowCommand = ShowCommand::kMinimize; }

    void GLFWWindow::Restore() { m_PendingShowCommand = ShowCommand::kRestore; }

    float GLFWWindow::GetTitleBarNativeButtonsWidth() const
    {
        return m_TitleBar ? m_TitleBar->GetNativeButtonsWidth() : 0.0f;
    }

    void GLFWWindow::SetTitleBarLayout(WindowTitleBarLayout&& _Layout)
    {
        if (m_TitleBar)
        {
            m_TitleBar->SetLayout(std::move(_Layout));
        }
    }

    bool GLFWWindow::IsCursorOnResizeBorder() const { return m_TitleBar && m_TitleBar->IsCursorOnResizeBorder(); }

    void GLFWWindow::OnUpdate()
    {
        ApplyPendingShowCommand();

        if (m_TitleBar)
        {
            m_TitleBar->OnUpdate();
        }

        glfwMakeContextCurrent(m_Window);
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    void GLFWWindow::ApplyPendingShowCommand()
    {
        switch (m_PendingShowCommand)
        {
            case ShowCommand::kNone: break;
            case ShowCommand::kMaximize: glfwMaximizeWindow(m_Window); break;
            case ShowCommand::kMinimize: glfwIconifyWindow(m_Window); break;
            case ShowCommand::kRestore: glfwRestoreWindow(m_Window); break;
        }

        m_PendingShowCommand = ShowCommand::kNone;
    }

    bool GLFWWindow::Init()
    {
#ifdef VEGA_PLATFORM_LINUX
        // ImGui multi-viewport cannot position secondary windows on Wayland, so X11 is forced
        // (XWayland when running under a Wayland session)
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif

        if (!glfwInit())
        {
            VEGA_CORE_CRITICAL("Failed to initialize GLFW!");
            return false;
        }

#ifdef VEGA_PLATFORM_LINUX
        LogWindowSystem();
#endif

        switch (m_Data.RendererAPI)
        {
            case RendererBackendApi::kVulkan: glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); break;
#ifdef _DEBUG
            case RendererBackendApi::kOpenGL: glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); break;
#endif
        }

        // Hidden until the title bar is set up, otherwise the system one flashes on start
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), NULL, NULL);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

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

        if (m_Data.IsUseCustomTitlebar)
        {
            m_TitleBar = GLFWTitleBar::Create(m_Window);
            m_Data.TitleBar = m_TitleBar.get();
        }
        VEGA_CORE_TRACE("Custom title bar: {}", m_TitleBar != nullptr);

        glfwShowWindow(m_Window);
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

            if (data.TitleBar)
            {
                data.TitleBar->OnMouseButton(_Button, _Action);
            }

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
