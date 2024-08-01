#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Core/Assert.hpp"
#include "Vega/Core/Window.hpp"
#include "Vega/Events/EventManager.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/Layers/LayerStack.hpp"
#include "Vega/Renderer/RendererBackend.hpp"

#include <functional>

int main(int argc, char** argv);

namespace LM
{

    struct ApplicationCommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            LM_CORE_ASSERT(index < Count);
            return Args[index];
        }
    };

    struct ApplicationProps
    {
        std::string Name = "LM Application";
        std::string WorkingDirectory;
        RendererBackend::API RendererAPI = RendererBackend::API::kVulkan;
        ApplicationCommandLineArgs CommandLineArgs;
    };

    class Application
    {
    public:
        Application(const ApplicationProps& _Props);
        virtual ~Application();

        void PushLayer(Ref<Layer> _Layer);
        void PushOverlay(Ref<Layer> _Layer);

        Ref<Window> GetWindow() { return m_Window; }

        void Close();

        static Application& Get() { return *s_Instance; }

        const ApplicationProps& GetProps() const { return m_Props; }

        const Ref<RendererBackend> GetRendererBackend() const { return m_RendererBackend; }

        template <typename T>
        const Ref<T> GetCastedRendererBackend() const
        {
            return std::dynamic_pointer_cast<T>(m_RendererBackend);
        }

    private:
        void Run();
        bool OnWindowClose(const WindowCloseEvent& _Event);
        bool OnWindowResize(const WindowResizeEvent& _Event);

    private:
        ApplicationProps m_Props;
        Ref<Window> m_Window;
        Ref<RendererBackend> m_RendererBackend;
        Ref<EventManager> m_EventManager;
        bool m_Running = true;
        bool m_Minimized = false;
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;

    private:
        static Application* s_Instance;
        friend int ::main(int argc, char** argv);
    };

    // To be defined in CLIENT
    Application* CreateApplication(ApplicationCommandLineArgs args);

}    // namespace LM
