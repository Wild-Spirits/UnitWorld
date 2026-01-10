#include "Application.hpp"

#include "Vega/Utils/Log.hpp"

#include <nfd.hpp>

#include <filesystem>

namespace Vega
{

    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationProps& _Props) : m_Props(_Props)
    {
        Log::Init();

        VEGA_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        NFD::Init();

        if (!_Props.WorkingDirectory.empty())
        {
            std::filesystem::current_path(_Props.WorkingDirectory);
        }

        m_EventManager = CreateRef<EventManager>();

        WindowProps windowProps = {
            .Title = _Props.Name,
            .Width = 1280u,
            .Height = 720u,
            .RendererAPI = _Props.RendererAPI,
            .IsUseCustomTitlebar = GetIsHasCutsomTitleBar(),
        };
        m_Window = Window::Create(windowProps);
        m_Window->SetEventManager(m_EventManager);

        RendererBackend::CreateReturnValue rendererBackend = RendererBackend::Create(_Props.RendererAPI);
        m_RendererBackendPlugin = rendererBackend.Plugin;
        m_RendererBackend = rendererBackend.Backend;
        m_RendererBackend->Init();
        m_RendererBackend->OnWindowCreate(m_Window);

        m_EventManager->Subscribe(EventHandler<WindowResizeEvent>(VEGA_BIND_EVENT_FN(OnWindowResize)));
        m_EventManager->Subscribe(EventHandler<WindowCloseEvent>(VEGA_BIND_EVENT_FN(OnWindowClose)));
    }    // namespace Vega

    Application::~Application()
    {
        m_LayerStack.Clear();

        for (auto& [name, manager] : m_Managers)
        {
            manager->Destroy();
        }
        m_Managers.clear();

        m_RendererBackend->OnWindowDestroy(m_Window);
        m_RendererBackend->Shutdown();

        NFD::Quit();
    }

    void Application::PushLayer(Ref<Layer> _Layer)
    {
        m_LayerStack.PushLayer(_Layer);

        const float model = 10;
        _Layer->OnAttach(m_EventManager);
    }

    void Application::PushOverlay(Ref<Layer> _Layer)
    {
        m_LayerStack.PushOverlay(_Layer);
        _Layer->OnAttach(m_EventManager);
    }

    void Application::AddManager(std::string_view _Name, Ref<Manager> _Manager)
    {
        VEGA_CORE_ASSERT(!IsHasManager(_Name), "Manager with this name already exists!");
        m_Managers[_Name.data()] = _Manager;
    }

    bool Application::IsHasManager(std::string_view _Name) const
    {
        return m_Managers.find(_Name.data()) != m_Managers.end();
    }

    Ref<Manager> Application::GetManager(std::string_view _Name)
    {
        VEGA_CORE_ASSERT(IsHasManager(_Name), "Manager with this name does not exist!");
        return m_Managers.at(_Name.data());
    }

    void Application::Close() { m_Running = false; }

    bool Application::GetIsHasCutsomTitleBar() const
    {
        bool result = m_Props.IsUseCustomTitlebar;
#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)
        result = result && true;
#else
        result = false;
#endif
        return result;
    }

    void Application::Run()
    {
        while (m_Running)
        {
            // float time = Time::GetTime();
            float time = 0.0f;
            float timestepSec = time - m_LastFrameTime;
            m_LastFrameTime = time;

            for (Ref<Layer> layer : m_LayerStack)
            {
                layer->OnUpdate();
            }

            if (!m_Minimized)
            {
                if (m_Resizing)
                {
                    m_RendererBackend->OnResize();
                    m_RendererBackend->FramePrepareWindowSurface();
                    m_Resizing = false;
                }
                else
                {
                    if (m_RendererBackend->FramePrepareWindowSurface())
                    {
                        m_RendererBackend->FrameCommandListBegin();

                        for (Ref<Layer> layer : m_LayerStack)
                        {
                            layer->OnRender();
                        }

                        if (m_GuiLayer && m_GuiLayer->BeginGuiFrame())
                        {
                            for (Ref<Layer> layer : m_LayerStack)
                            {
                                layer->OnGuiRender();
                            }
                            m_GuiLayer->EndGuiFrame();
                        }

                        // TODO: Use rendergraph here
                        m_RendererBackend->TmpRendergraphExecute();

                        m_RendererBackend->FrameCommandListEnd();
                        m_RendererBackend->FrameSubmit();
                        m_RendererBackend->FramePresent();
                    }
                    m_GuiLayer->OnExternalViewportsRender();
                }
            }

            m_Window->OnUpdate();

            m_EventManager->DispatchEvents();
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
        m_Resizing = true;

        return false;
    }

}    // namespace Vega
