#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Events/EventManager.hpp"
#include "Vega/Renderer/RendererBackendApi.hpp"
#include "glm/fwd.hpp"
#include "glm/vec2.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Vega
{
    struct WindowProps
    {
        std::string Title = "Vega";
        uint32_t Width = 1280u;
        uint32_t Height = 720u;
        RendererBackendApi RendererAPI = RendererBackendApi::kNone;
        bool IsUseCustomTitlebar = true;
    };

    // Custom title bar layout reported by the client every frame. Coordinates are in window space (the same as
    // GetCursorInWindowPosition). The platform uses it to decide which points of the title bar drag the window.
    struct WindowTitleBarLayout
    {
        struct ItemRect
        {
            glm::vec2 Min;
            glm::vec2 Max;
        };

        float Height = 0.0f;
        // Menus, buttons and other items that must receive the mouse instead of dragging the window
        std::vector<ItemRect> InteractiveItems;
    };

    class Window
    {
    public:
        virtual ~Window() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual glm::dvec2 GetCursorInWindowPosition() const = 0;
        virtual std::string_view GetTitle() const = 0;
        virtual float GetMonitorScale() const = 0;

        virtual bool IsWindowMaximized() const = 0;

        // State changes are applied in OnUpdate, outside of the frame rendering
        virtual void Maximize() = 0;
        virtual void Minimize() = 0;
        virtual void Restore() = 0;

        // The system caption is removed and the client draws its own title bar. Can be false even if
        // WindowProps::IsUseCustomTitlebar is set: the platform may not support it (e.g. Wayland)
        virtual bool IsCustomTitleBar() const = 0;
        // System window buttons are still shown (macOS traffic lights): the client must not draw its own ones
        virtual bool IsTitleBarHasNativeButtons() const = 0;
        // Width reserved for the native window buttons on the left side of the title bar
        virtual float GetTitleBarNativeButtonsWidth() const = 0;
        virtual void SetTitleBarLayout(WindowTitleBarLayout&& _Layout) = 0;
        // The cursor is over a resize border handled by the platform: the client must not change the cursor shape
        virtual bool IsCursorOnResizeBorder() const = 0;

        virtual void OnUpdate() = 0;

        virtual void SetEventManager(Ref<EventManager> _EventManager) = 0;

        virtual void* GetNativeWindow() const = 0;

        static Ref<Window> Create(const WindowProps& _Props = WindowProps());
    };

}    // namespace Vega
