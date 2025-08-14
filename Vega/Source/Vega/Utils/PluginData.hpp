#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Core/Application.hpp"

namespace Vega
{

    class PluginData
    {
    public:
        typedef void (*SetPluginGlobalsFunc)(Vega::PluginData* _PluginData);

        PluginData();

        void SetPluginData();

    protected:
        Application* m_Application;
        Ref<Logger> m_CoreLogger;
        Ref<Logger> m_ClientLogger;
    };

}    // namespace Vega
