#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Events/EventManager.hpp"
#include "Vega/Renderer/RendererBackendApi.hpp"
#include "glm/fwd.hpp"

#include <cstdint>
#include <string>

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

        virtual void Maximize() = 0;
        virtual void Minimize() = 0;
        virtual void Restore() = 0;

        virtual void OnUpdate() = 0;

        virtual void SetEventManager(Ref<EventManager> _EventManager) = 0;

        virtual void* GetNativeWindow() const = 0;

        static Ref<Window> Create(const WindowProps& _Props = WindowProps());
    };

}    // namespace Vega
