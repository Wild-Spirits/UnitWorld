#pragma once

#include "Vega/Core/Base.hpp"
#include "Vega/Core/Window.hpp"

#include "glm/vec2.hpp"

#include <cstdint>
#include <utility>

struct GLFWwindow;

namespace Vega
{

    enum class TitleBarHit
    {
        kNone,    // Outside of the window
        kClient,
        kCaption,
        kResizeLeft,
        kResizeRight,
        kResizeTop,
        kResizeBottom,
        kResizeTopLeft,
        kResizeTopRight,
        kResizeBottomLeft,
        kResizeBottomRight,
    };

    typedef uint32_t TitleBarResizeBorderFlags;
    namespace TitleBarResizeBorderFlagBits
    {
        enum : TitleBarResizeBorderFlags
        {
            kNone = 0,
            kLeft = BIT(0),
            kRight = BIT(1),
            kTop = BIT(2),
            kBottom = BIT(3),
            kAll = kLeft | kRight | kTop | kBottom,
        };
    }    // namespace TitleBarResizeBorderFlagBits

    // Platform part of the custom title bar: removes the system caption and makes the client title bar behave
    // like a native one (window dragging, resizing, double click). The title bar itself is drawn by the client.
    class GLFWTitleBar
    {
    public:
        GLFWTitleBar(GLFWwindow* _Window) : m_Window(_Window) { }
        virtual ~GLFWTitleBar() = default;

        // Returns nullptr if the custom title bar isn't supported: the system one is used then
        static Scope<GLFWTitleBar> Create(GLFWwindow* _Window);

        // Called every frame before polling the window events
        virtual void OnUpdate() { }
        // Called from the GLFW mouse button callback
        virtual void OnMouseButton(int _Button, int _Action) { }

        virtual bool IsHasNativeButtons() const { return false; }
        virtual float GetNativeButtonsWidth() const { return 0.0f; }

        void SetLayout(WindowTitleBarLayout&& _Layout) { m_Layout = std::move(_Layout); }

        // _Point is in window coordinates
        TitleBarHit HitTest(const glm::vec2& _Point) const;
        bool IsCursorOnResizeBorder() const;

        static bool IsResizeHit(TitleBarHit _Hit);

    protected:
        // Resize borders located inside the client area and handled by the title bar (not by the system)
        virtual TitleBarResizeBorderFlags GetClientResizeBorders() const { return TitleBarResizeBorderFlagBits::kAll; }
        // In window coordinates
        virtual float GetResizeBorderWidth() const;

        bool IsPointInCaption(const glm::vec2& _Point) const;

        glm::vec2 GetCursorPos() const;
        glm::vec2 GetWindowSize() const;
        bool IsMaximized() const;
        bool IsResizable() const;

    protected:
        GLFWwindow* m_Window = nullptr;
        WindowTitleBarLayout m_Layout;
    };

}    // namespace Vega
