#include "Window.hpp"

#include "Platform/OpenGL/Core/GLFWWindow.hpp"

namespace LM
{

    Ref<Window> Window::Create(const WindowProps& _Props)
    {
        return CreateRef<GLFWWindow>(_Props);
    }

}    // namespace LM
