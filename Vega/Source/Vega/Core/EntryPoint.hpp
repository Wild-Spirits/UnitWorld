#pragma once

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Base.hpp"

extern Vega::Application* Vega::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
    std::setlocale(LC_CTYPE, ".UTF8");

    auto App = Vega::CreateApplication({ argc, argv });

    App->Run();

    delete App;

    return 0;
}
