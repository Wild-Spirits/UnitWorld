#pragma once

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Base.hpp"

extern LM::Application* LM::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
    std::setlocale(LC_CTYPE, ".UTF8");

    auto App = LM::CreateApplication({ argc, argv });

    App->Run();

    delete App;

    return 0;
}
