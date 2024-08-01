#include "Window.hpp"

#include "Vega/Core/Assert.hpp"

#include "Platform/Desktop/Core/GLFWWindow.hpp"
#include "Platform/Platform.hpp"

namespace LM
{

    Ref<Window> Window::Create(const WindowProps& _Props)
    {
#ifdef LM_PLATFORM_DESKTOP
        return CreateRef<GLFWWindow>(_Props);
#endif

        LM_CORE_ASSERT(false, "Unknown Platform!")
        return nullptr;
    }

}    // namespace LM
