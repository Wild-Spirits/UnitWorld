#include "Window.hpp"

#include "Vega/Core/Assert.hpp"

#include "Platform/Desktop/Core/GLFWWindow.hpp"
#include "Platform/Platform.hpp"

namespace Vega
{

    Ref<Window> Window::Create(const WindowProps& _Props)
    {
#ifdef VEGA_PLATFORM_DESKTOP
        return CreateRef<GLFWWindow>(_Props);
#endif

        VEGA_CORE_ASSERT(false, "Unknown Platform!")
        return nullptr;
    }

}    // namespace Vega
