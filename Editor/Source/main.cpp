#include <Vega/Core/EntryPoint.hpp>

#include "EditorLayer.hpp"

#include "args.hxx"

#include <Vega/Layers/ImGuiLayer.hpp>
#include <Vega/Utils/utf8.hpp>

namespace LM
{

    class App : public Application
    {
    public:
        App(const ApplicationProps& _Props) : Application(_Props)
        {
            PushLayer(CreateRef<EditorLayer>());
            PushOverlay(CreateRef<ImGuiLayer>());
        }
    };

    Application* CreateApplication(ApplicationCommandLineArgs _Args)
    {
        args::ArgumentParser argsParser("Unit World Editor", "Unit World Editor");
        args::HelpFlag help(argsParser, "help", "Display this help menu", { 'h', "help" });
        args::Group rendererAPIGroup(argsParser, "Renderer API:", args::Group::Validators::AtMostOne);
        args::Flag rendererAPIVulkan(rendererAPIGroup, "Vulkan", "Use Vulkan renderer API", { "vulkan" });
        args::Flag rendererAPIOpenGL(rendererAPIGroup, "OpenGL", "Use OpenGL renderer API", { "opengl" });
        args::Flag rendererAPIDirectX(rendererAPIGroup, "DirectX", "Use DirectX renderer API", { "directx" });

        try
        {
            argsParser.ParseCLI(_Args.Count, _Args.Args);
        }
        catch (args::Help)
        {
            std::cout << argsParser;
            return nullptr;
        }
        catch (args::ParseError e)
        {
            std::cerr << e.what() << std::endl;
            std::cerr << argsParser;
            return nullptr;
        }
        catch (args::ValidationError e)
        {
            std::cerr << e.what() << std::endl;
            std::cerr << argsParser;
            return nullptr;
        }

        ApplicationProps props;
        props.Name = "Unit World Editor";
        props.CommandLineArgs = _Args;
        props.WorkingDirectory = U8_RES(RES_FOLDER);
        props.RendererAPI = RendererBackend::API::kOpenGL;

        if (rendererAPIOpenGL)
        {
            props.RendererAPI = RendererBackend::API::kOpenGL;
        }
        if (rendererAPIDirectX)
        {
            props.RendererAPI = RendererBackend::API::kDirectX;
        }
        if (rendererAPIVulkan)
        {
            props.RendererAPI = RendererBackend::API::kVulkan;
        }

        return new App(props);
    }

}    // namespace LM
