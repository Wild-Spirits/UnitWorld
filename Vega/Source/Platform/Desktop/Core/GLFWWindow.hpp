#pragma once

#include "Platform/Platform.hpp"

#include "Vega/Core/Window.hpp"

struct GLFWwindow;

namespace Vega
{

    class GLFWWindow : public Window
    {
    public:
        struct WindowData : WindowProps
        {
            WindowData(const WindowProps& _Props) : WindowProps(_Props) { }
            Ref<EventManager> EventManager;
            float MonitorScale = 1.0f;
#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)
            void* OriginalProc = nullptr;
#endif
        };

    public:
        GLFWWindow(const WindowProps& _Props);
        virtual ~GLFWWindow();

        virtual uint32_t GetWidth() const override { return m_Data.Width; }
        virtual uint32_t GetHeight() const override { return m_Data.Height; }
        virtual glm::dvec2 GetCursorInWindowPosition() const override;
        virtual std::string_view GetTitle() const override { return m_Data.Title; }
        virtual float GetMonitorScale() const override { return m_Data.MonitorScale; }

        virtual bool IsWindowMaximized() const override;

        virtual void Maximize() override;
        virtual void Minimize() override;
        virtual void Restore() override;

        virtual void OnUpdate() override;

        void SetEventManager(Ref<EventManager> _EventManager) override { m_Data.EventManager = _EventManager; }

        virtual void* GetNativeWindow() const override { return m_Window; }

    protected:
        bool Init();
        void SetCallbacks();

    protected:
        GLFWwindow* m_Window;

        WindowData m_Data;
    };

}    // namespace Vega
