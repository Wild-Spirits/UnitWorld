#include "GLFWTitleBar.hpp"

#include "Platform/Platform.hpp"

#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)
    #include "Platform/Windows/Core/WinTitleBar.hpp"
#elif defined(VEGA_PLATFORM_LINUX_DESKTOP)
    #include "Platform/Linux/Core/LinuxTitleBar.hpp"
#elif defined(VEGA_PLATFORM_MACOS_DESKTOP)
    #include "Platform/MacOS/Core/MacOSTitleBar.hpp"
#endif

#include <GLFW/glfw3.h>

namespace Vega
{

    // Base resize border width, scaled by the window content scale
    constexpr float kResizeBorderWidth = 6.0f;

    Scope<GLFWTitleBar> GLFWTitleBar::Create(GLFWwindow* _Window)
    {
#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)
        return CreateScope<WinTitleBar>(_Window);
#elif defined(VEGA_PLATFORM_LINUX_DESKTOP)
        if (LinuxTitleBar::IsSupported())
        {
            return CreateScope<LinuxTitleBar>(_Window);
        }
        return nullptr;
#elif defined(VEGA_PLATFORM_MACOS_DESKTOP)
        return CreateScope<MacOSTitleBar>(_Window);
#else
        return nullptr;
#endif
    }

    TitleBarHit GLFWTitleBar::HitTest(const glm::vec2& _Point) const
    {
        glm::vec2 windowSize = GetWindowSize();
        if (_Point.x < 0.0f || _Point.y < 0.0f || _Point.x >= windowSize.x || _Point.y >= windowSize.y)
        {
            return TitleBarHit::kNone;
        }

        TitleBarResizeBorderFlags borders = GetClientResizeBorders();
        if (borders != TitleBarResizeBorderFlagBits::kNone && IsResizable() && !IsMaximized())
        {
            float borderWidth = GetResizeBorderWidth();

            bool isLeft = (borders & TitleBarResizeBorderFlagBits::kLeft) && _Point.x < borderWidth;
            bool isRight = (borders & TitleBarResizeBorderFlagBits::kRight) && _Point.x >= windowSize.x - borderWidth;
            bool isTop = (borders & TitleBarResizeBorderFlagBits::kTop) && _Point.y < borderWidth;
            bool isBottom = (borders & TitleBarResizeBorderFlagBits::kBottom) && _Point.y >= windowSize.y - borderWidth;

            if (isLeft || isRight || isTop || isBottom)
            {
                // Corners are a bit larger than borders to be easier to grab
                float cornerSize = borderWidth * 2.0f;
                bool isNearLeft = (borders & TitleBarResizeBorderFlagBits::kLeft) && _Point.x < cornerSize;
                bool isNearRight =
                    (borders & TitleBarResizeBorderFlagBits::kRight) && _Point.x >= windowSize.x - cornerSize;
                bool isNearTop = (borders & TitleBarResizeBorderFlagBits::kTop) && _Point.y < cornerSize;
                bool isNearBottom =
                    (borders & TitleBarResizeBorderFlagBits::kBottom) && _Point.y >= windowSize.y - cornerSize;

                if (isNearTop && isNearLeft)
                {
                    return TitleBarHit::kResizeTopLeft;
                }
                if (isNearTop && isNearRight)
                {
                    return TitleBarHit::kResizeTopRight;
                }
                if (isNearBottom && isNearLeft)
                {
                    return TitleBarHit::kResizeBottomLeft;
                }
                if (isNearBottom && isNearRight)
                {
                    return TitleBarHit::kResizeBottomRight;
                }
                if (isLeft)
                {
                    return TitleBarHit::kResizeLeft;
                }
                if (isRight)
                {
                    return TitleBarHit::kResizeRight;
                }
                if (isTop)
                {
                    return TitleBarHit::kResizeTop;
                }
                return TitleBarHit::kResizeBottom;
            }
        }

        if (IsPointInCaption(_Point))
        {
            return TitleBarHit::kCaption;
        }

        return TitleBarHit::kClient;
    }

    bool GLFWTitleBar::IsCursorOnResizeBorder() const { return IsResizeHit(HitTest(GetCursorPos())); }

    bool GLFWTitleBar::IsResizeHit(TitleBarHit _Hit)
    {
        return _Hit != TitleBarHit::kNone && _Hit != TitleBarHit::kClient && _Hit != TitleBarHit::kCaption;
    }

    float GLFWTitleBar::GetResizeBorderWidth() const
    {
        float scaleX = 1.0f;
        glfwGetWindowContentScale(m_Window, &scaleX, nullptr);
        return kResizeBorderWidth * scaleX;
    }

    bool GLFWTitleBar::IsPointInCaption(const glm::vec2& _Point) const
    {
        if (_Point.y < 0.0f || _Point.y >= m_Layout.Height || _Point.x < 0.0f || _Point.x >= GetWindowSize().x)
        {
            return false;
        }

        for (const WindowTitleBarLayout::ItemRect& item : m_Layout.InteractiveItems)
        {
            if (_Point.x >= item.Min.x && _Point.x < item.Max.x && _Point.y >= item.Min.y && _Point.y < item.Max.y)
            {
                return false;
            }
        }

        return true;
    }

    glm::vec2 GLFWTitleBar::GetCursorPos() const
    {
        double cursorX, cursorY;
        glfwGetCursorPos(m_Window, &cursorX, &cursorY);
        return { static_cast<float>(cursorX), static_cast<float>(cursorY) };
    }

    glm::vec2 GLFWTitleBar::GetWindowSize() const
    {
        int width, height;
        glfwGetWindowSize(m_Window, &width, &height);
        return { static_cast<float>(width), static_cast<float>(height) };
    }

    bool GLFWTitleBar::IsMaximized() const { return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED) == GLFW_TRUE; }

    bool GLFWTitleBar::IsResizable() const { return glfwGetWindowAttrib(m_Window, GLFW_RESIZABLE) == GLFW_TRUE; }

}    // namespace Vega
