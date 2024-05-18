#pragma once

#include "Vega/Core/Window.hpp"

struct GLFWwindow;

namespace LM
{

    class GLFWWindow : public Window
    {
    public:
        GLFWWindow(const WindowProps& _Props);
        virtual ~GLFWWindow();

        virtual uint32_t GetWidth() const override { return m_Data.Width; }
        virtual uint32_t GetHeight() const override { return m_Data.Height; }
        virtual float GetMonitorScale() const override { return m_Data.MonitorScale; }

        virtual void OnUpdate() override;

        void SetEventManager(Ref<EventManager> _EventManager) override { m_Data.EventManager = _EventManager; }

        virtual void* GetNativeWindow() const override { return m_Window; }

    protected:
        bool Init();
        void SetCallbacks();

    protected:
        struct WindowData : WindowProps
        {
            WindowData(const WindowProps& _Props) : WindowProps(_Props) { }
            Ref<EventManager> EventManager;
            float MonitorScale = 1.0f;
        };

        GLFWwindow* m_Window;

        WindowData m_Data;
    };

}    // namespace LM
