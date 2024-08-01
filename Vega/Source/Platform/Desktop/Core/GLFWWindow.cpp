#include "GLFWWindow.hpp"

#include "Vega/Events/KeyEvent.hpp"
#include "Vega/Events/MouseEvent.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/Utils/Log.hpp"

#include <GLFW/glfw3.h>

namespace LM
{

    bool glfw_get_window_monitor(GLFWmonitor** monitor, GLFWwindow* window);

    GLFWWindow::GLFWWindow(const WindowProps& _Props) : m_Data(_Props) { Init(); }

    GLFWWindow::~GLFWWindow() { glfwDestroyWindow(m_Window); }

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
            LM_CORE_CRITICAL("Failed to initialize GLFW!");
            return false;
        }

        switch (m_Data.RendererAPI)
        {
            case RendererBackend::API::kVulkan: glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); break;
#ifdef _DEBUG
            case RendererBackend::API::kOpenGL: glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); break;
#endif
        }

        m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), NULL, NULL);

        if (!m_Window)
        {
            LM_CORE_CRITICAL("Failed to create window!");
            return false;
        }

        GLFWmonitor* nowMonitor = NULL;
        if (!glfw_get_window_monitor(&nowMonitor, m_Window))
        {
            nowMonitor = glfwGetPrimaryMonitor();
        }
        glfwGetMonitorContentScale(nowMonitor, NULL, &m_Data.MonitorScale);
        LM_CORE_TRACE("Current monitor: {}", static_cast<void*>(nowMonitor));
        LM_CORE_TRACE("Current monitor scale: {}", m_Data.MonitorScale);

        glfwMakeContextCurrent(m_Window);
        glfwSetWindowUserPointer(m_Window, &m_Data);

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

        glfwSwapInterval(1);

        return true;
    }

    void GLFWWindow::SetCallbacks()
    {
        glfwSetWindowPosCallback(m_Window, [](GLFWwindow* _Window, int _PosX, int _PosY) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

            GLFWmonitor* nowMonitor = NULL;
            if (!glfw_get_window_monitor(&nowMonitor, _Window))
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
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);
            data.Width = _Width;
            data.Height = _Height;

            {
                Scope<WindowResizeEvent> event = CreateScope<WindowResizeEvent>(_Width, _Height);
                data.EventManager->QueueEvent(std::move(event));
            }

            {
                GLFWmonitor* nowMonitor = NULL;
                if (!glfw_get_window_monitor(&nowMonitor, _Window))
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
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

            Scope<WindowCloseEvent> event = CreateScope<WindowCloseEvent>();

            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* _Window, int _Key, int _ScanCode, int _Action, int _Mods) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

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
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

            Scope<KeyTypedEvent> event = CreateScope<KeyTypedEvent>(_Keycode);
            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* _Window, int _Button, int _Action, int _Mods) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

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
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

            Scope<MouseScrolledEvent> event =
                CreateScope<MouseScrolledEvent>(static_cast<float>(_OffsetX), static_cast<float>(_OffsetY));
            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* _Window, double _PosX, double _PosY) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

            Scope<MouseMovedEvent> event =
                CreateScope<MouseMovedEvent>(static_cast<float>(_PosX), static_cast<float>(_PosY));
            data.EventManager->QueueEvent(std::move(event));
        });
    }

    bool glfw_get_window_monitor(GLFWmonitor** monitor, GLFWwindow* window)
    {
        bool success = false;

        int window_rectangle[4] = { 0 };
        glfwGetWindowPos(window, &window_rectangle[0], &window_rectangle[1]);
        glfwGetWindowSize(window, &window_rectangle[2], &window_rectangle[3]);

        int monitors_size = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&monitors_size);

        GLFWmonitor* closest_monitor = NULL;
        int max_overlap_area = 0;

        for (int i = 0; i < monitors_size; ++i)
        {
            int monitor_position[2] = { 0 };
            glfwGetMonitorPos(monitors[i], &monitor_position[0], &monitor_position[1]);

            const GLFWvidmode* monitor_video_mode = glfwGetVideoMode(monitors[i]);

            int monitor_rectangle[4] = {
                monitor_position[0],
                monitor_position[1],
                monitor_video_mode->width,
                monitor_video_mode->height,
            };

            if (!(((window_rectangle[0] + window_rectangle[2]) < monitor_rectangle[0]) ||
                  (window_rectangle[0] > (monitor_rectangle[0] + monitor_rectangle[2])) ||
                  ((window_rectangle[1] + window_rectangle[3]) < monitor_rectangle[1]) ||
                  (window_rectangle[1] > (monitor_rectangle[1] + monitor_rectangle[3]))))
            {
                int intersection_rectangle[4] = { 0 };

                // x, width
                if (window_rectangle[0] < monitor_rectangle[0])
                {
                    intersection_rectangle[0] = monitor_rectangle[0];

                    if ((window_rectangle[0] + window_rectangle[2]) < (monitor_rectangle[0] + monitor_rectangle[2]))
                    {
                        intersection_rectangle[2] =
                            (window_rectangle[0] + window_rectangle[2]) - intersection_rectangle[0];
                    }
                    else
                    {
                        intersection_rectangle[2] = monitor_rectangle[2];
                    }
                }
                else
                {
                    intersection_rectangle[0] = window_rectangle[0];

                    if ((monitor_rectangle[0] + monitor_rectangle[2]) < (window_rectangle[0] + window_rectangle[2]))
                    {
                        intersection_rectangle[2] =
                            (monitor_rectangle[0] + monitor_rectangle[2]) - intersection_rectangle[0];
                    }
                    else
                    {
                        intersection_rectangle[2] = window_rectangle[2];
                    }
                }

                // y, height
                if (window_rectangle[1] < monitor_rectangle[1])
                {
                    intersection_rectangle[1] = monitor_rectangle[1];

                    if ((window_rectangle[1] + window_rectangle[3]) < (monitor_rectangle[1] + monitor_rectangle[3]))
                    {
                        intersection_rectangle[3] =
                            (window_rectangle[1] + window_rectangle[3]) - intersection_rectangle[1];
                    }
                    else
                    {
                        intersection_rectangle[3] = monitor_rectangle[3];
                    }
                }
                else
                {
                    intersection_rectangle[1] = window_rectangle[1];

                    if ((monitor_rectangle[1] + monitor_rectangle[3]) < (window_rectangle[1] + window_rectangle[3]))
                    {
                        intersection_rectangle[3] =
                            (monitor_rectangle[1] + monitor_rectangle[3]) - intersection_rectangle[1];
                    }
                    else
                    {
                        intersection_rectangle[3] = window_rectangle[3];
                    }
                }

                // int overlap_area = intersection_rectangle[3] * intersection_rectangle[4];
                int overlap_area = intersection_rectangle[2] * intersection_rectangle[3];

                if (overlap_area > max_overlap_area)
                {
                    closest_monitor = monitors[i];
                    max_overlap_area = overlap_area;
                }
            }
        }

        if (closest_monitor)
        {
            *monitor = closest_monitor;
            success = true;
        }

        // true: monitor contains the monitor the window is most on
        // false: monitor is unmodified
        return success;
    }

}    // namespace LM
