#include "PluginLibrary.hpp"

namespace Vega
{

    PluginLibrary::PluginLibrary(const PluginLibraryProps& _PluginLibraryProps)
        : m_PluginLibraryProps(_PluginLibraryProps)
    {
        LoadPlugin();
    }

    PluginLibrary::~PluginLibrary() { UnloadPlugin(); }

    void PluginLibrary::ReloadPlugin()
    {
        UnloadPlugin();
        LoadPlugin();
    }

    std::filesystem::path PluginLibrary::GetDefaultPluginPath()
    {
        return std::filesystem::path(BIN_FOLDER) / std::filesystem::path("VegaPlugins");
    }

}    // namespace Vega
