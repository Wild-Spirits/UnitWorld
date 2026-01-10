#pragma once

#include "Vega/Core/Assert.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/Core/Window.hpp"
#include "Vega/Events/EventManager.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/Layers/GuiLayer.hpp"
#include "Vega/Layers/LayerStack.hpp"
#include "Vega/Managers/Manager.hpp"
#include "Vega/Plugins/PluginLibrary.hpp"
#include "Vega/Renderer/RendererBackend.hpp"


#include <string>

int main(int argc, char** argv);

namespace Vega
{

    struct ApplicationCommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            VEGA_CORE_ASSERT(index < Count);
            return Args[index];
        }
    };

    struct ApplicationProps
    {
        std::string Name = "Vega Application";
        std::string WorkingDirectory;
        std::string BinaryDirectory;
        RendererBackendApi RendererAPI = RendererBackendApi::kVulkan;
        bool IsUseCustomTitlebar = true;
        ApplicationCommandLineArgs CommandLineArgs;
    };

    class Application
    {
    public:
        Application(const ApplicationProps& _Props);
        virtual ~Application();

        void PushLayer(Ref<Layer> _Layer);
        void PushOverlay(Ref<Layer> _Layer);

        void AddManager(std::string_view _Name, Ref<Manager> _Manager);
        bool IsHasManager(std::string_view _Name) const;
        Ref<Manager> GetManager(std::string_view _Name);

        Ref<Window> GetWindow() { return m_Window; }

        void Close();

        static Application& Get() { return *s_Instance; }
        static void SetPluginApp(Application* _App) { s_Instance = _App; }

        const ApplicationProps& GetProps() const { return m_Props; }

        const Ref<RendererBackend> GetRendererBackend() const { return m_RendererBackend; }

        bool GetIsHasCutsomTitleBar() const;

        inline void SetIsMainMenuAnyItemHovered(bool _IsHovered) { m_IsMainMenuAnyItemHovered = _IsHovered; }
        inline bool GetIsMainMenuAnyItemHovered() const { return m_IsMainMenuAnyItemHovered; }

        inline void SetMainMenuFrameHeight(float _Height) { m_MainMenuFrameHeight = _Height; }
        inline float GetMainMenuFrameHeight() const { return m_MainMenuFrameHeight; }

        // template <typename T>
        // const Ref<T> GetCastedRendererBackend() const
        // {
        //     return std::dynamic_pointer_cast<T>(m_RendererBackend);
        // }

    protected:
        void Run();
        bool OnWindowClose(const WindowCloseEvent& _Event);
        bool OnWindowResize(const WindowResizeEvent& _Event);

    protected:
        Ref<GuiLayer> m_GuiLayer;

    protected:
        ApplicationProps m_Props;
        Ref<Window> m_Window;

        Ref<PluginLibrary> m_RendererBackendPlugin;
        Ref<RendererBackend> m_RendererBackend;

        Ref<EventManager> m_EventManager;
        std::unordered_map<std::string, Ref<Manager>> m_Managers;

        bool m_Running = true;
        bool m_Minimized = false;
        bool m_Resizing = false;
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;

        bool m_IsMainMenuAnyItemHovered = false;
        float m_MainMenuFrameHeight = 48.0f;

    protected:
        static Application* s_Instance;
        friend int ::main(int argc, char** argv);
    };

    // To be defined in CLIENT
    Application* CreateApplication(ApplicationCommandLineArgs args);

}    // namespace Vega
