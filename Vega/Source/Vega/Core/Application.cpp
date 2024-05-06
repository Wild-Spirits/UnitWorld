#include "Application.hpp"

#include "Vega/Core/Inputs.hpp"
#include "Vega/Utils/Log.hpp"

#include <nfd.hpp>

#include <filesystem>

namespace LM
{

    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationProps& _Props) : m_Props(_Props)
    {
        LM_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        Log::Init();
        NFD_Init();

        // Set working directory here
        if (!_Props.WorkingDirectory.empty())
        {
            std::filesystem::current_path(_Props.WorkingDirectory);
        }

        m_EventManager = CreateRef<EventManager>();
        m_Window = Window::Create(WindowProps { _Props.Name });
        m_Window->SetEventManager(m_EventManager);

        m_EventManager->Subscribe(EventHandler<WindowResizeEvent>(LM_BIND_EVENT_FN(OnWindowResize)));
        m_EventManager->Subscribe(EventHandler<WindowCloseEvent>(LM_BIND_EVENT_FN(OnWindowClose)));
    }

    Application::~Application() { NFD_Quit(); }

    void Application::PushLayer(Ref<Layer> _Layer)
    {
        m_LayerStack.PushLayer(_Layer);
        _Layer->OnAttach(m_EventManager);
    }

    void Application::PushOverlay(Ref<Layer> _Layer)
    {
        m_LayerStack.PushOverlay(_Layer);
        _Layer->OnAttach(m_EventManager);
    }

    void Application::Close() { m_Running = false; }

    void Application::Run()
    {
        while (m_Running)
        {
            // float time = Time::GetTime();
            float time = 0.0f;
            float timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            for (Ref<Layer> layer : m_LayerStack)
            {
                layer->OnUpdate();
            }

            if (!m_Minimized)
            {
                for (Ref<Layer> layer : m_LayerStack)
                {
                    layer->OnRender();
                }
            }

            m_Window->OnUpdate();
        }
    }

    bool Application::OnWindowClose(const WindowCloseEvent& _Event)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(const WindowResizeEvent& _Event)
    {
        if (_Event.GetWidth() == 0 || _Event.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;

        return false;
    }

}    // namespace LM
