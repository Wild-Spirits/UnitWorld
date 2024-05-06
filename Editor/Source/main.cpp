#include <Vega/Core/EntryPoint.hpp>

#include <Vega/Utils/utf8.hpp>

#include "EditorLayer.hpp"

namespace LM
{

    class App : public Application
    {
    public:
        App(const ApplicationProps& _Props) : Application(_Props) { PushLayer(CreateRef<EditorLayer>()); }
    };

    Application* CreateApplication(ApplicationCommandLineArgs _Args)
    {
        ApplicationProps props;
        props.Name = "Unit World Editor";
        props.CommandLineArgs = _Args;
        props.WorkingDirectory = U8_RES(RES_FOLDER);

        return new App(props);
    }

}    // namespace LM
