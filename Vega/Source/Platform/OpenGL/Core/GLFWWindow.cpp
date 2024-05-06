#include "GLFWWindow.hpp"

#include "Vega/Events/KeyEvent.hpp"
#include "Vega/Events/MouseEvent.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/Utils/Log.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace LM
{

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

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), NULL, NULL);

        if (!m_Window)
        {
            LM_CORE_CRITICAL("Failed to create window!");
            return false;
        }

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
        // glfwSetDropCallback(m_Window, droping_paths);

        // glfwSetErrorCallback
        // glfwSetFramebufferSizeCallback
        // glfwSetDropCallback

        glfwSwapInterval(1);

        if (glewInit() != GLEW_OK)
        {
            LM_CORE_CRITICAL("Could not initialise GLEW!");
            return false;
        }

        glEnable(GL_BLEND);
        // glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        LM_CORE_TRACE("OpenGL version: ", glGetString(GL_VERSION));
        LM_CORE_TRACE("OpenGL vendor: ", glGetString(GL_VENDOR));

        return true;
    }

    void GLFWWindow::SetCallbacks()
    {
        // glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* _Window, int _Width, int _Height)
        //{
        //	WindowData* Data = (WindowData*)glfwGetWindowUserPointer(_Window);
        //	Data->Width = _Width;
        //	Data->Height = _Height;
        //	Data->EQueue->Add(CreateRef<WindowResizeEvent>(_Width, _Height));
        // });

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* _Window, int _Width, int _Height) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);
            data.Width = _Width;
            data.Height = _Height;

            Scope<WindowResizeEvent> event = CreateScope<WindowResizeEvent>(_Width, _Height);
            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* _Window) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_Window);

            Scope<WindowCloseEvent> event = CreateScope<WindowCloseEvent>();
            data.EventManager->QueueEvent(std::move(event));
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* _Window, int _Key, int _Scancode, int _Action, int _Mods) {
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

}    // namespace LM
