#include "PluginData.hpp"

Vega::PluginData::PluginData()
{
    m_Application = &Application::Get();
    m_CoreLogger = Log::GetCoreLogger();
    m_ClientLogger = Log::GetClientLogger();
}

void Vega::PluginData::SetPluginData()
{
    Log::SetPluginCoreLogger(m_CoreLogger);
    Log::SetPluginClientLogger(m_ClientLogger);
    Application::SetPluginApp(m_Application);
}
