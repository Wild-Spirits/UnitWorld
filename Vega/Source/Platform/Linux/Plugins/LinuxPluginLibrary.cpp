#include "Vega/Plugins/PluginLibrary.hpp"

#include "Vega/Core/Assert.hpp"

#include "Vega/Utils/PluginData.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_LINUX

    #include <dlfcn.h>

namespace Vega
{

    void PluginLibrary::LoadPlugin()
    {
        std::filesystem::path pluginFilepath =
            std::filesystem::path(m_PluginLibraryProps.PluginPath) /
            std::filesystem::path(std::format("{}.so", m_PluginLibraryProps.PluginName));

        m_LibraryHandle = dlopen(pluginFilepath.string().c_str(), RTLD_NOW | RTLD_LOCAL);

        if (!m_LibraryHandle)
        {
            VEGA_CORE_CRITICAL("Unable to load library: {}: {}", pluginFilepath.string(), dlerror());
            VEGA_CORE_ASSERT(false, "Unable to load library!");
        }

        if (m_PluginLibraryProps.IsNeedSetPluginGlobals)
        {
            PluginData::SetPluginGlobalsFunc setPluginGlobalsFunc =
                LoadFunction<PluginData::SetPluginGlobalsFunc>("SetPluginGlobals");

            if (!setPluginGlobalsFunc)
            {
                VEGA_CORE_CRITICAL("Unable to load SetPluginGlobals function in: {}", pluginFilepath.string());
                VEGA_CORE_ASSERT(false, "Unable to load library!");
            }

            PluginData pluginData;
            setPluginGlobalsFunc(&pluginData);
        }
    }

    void PluginLibrary::UnloadPlugin()
    {
        if (m_LibraryHandle)
        {
            dlclose(m_LibraryHandle);
            m_LibraryHandle = nullptr;
        }
    }

    PluginLibrary::FuncAddr PluginLibrary::LoadFunctionAddr(std::string_view _FunctionName)
    {
        void* functionAddress = dlsym(m_LibraryHandle, std::string(_FunctionName).c_str());
        return reinterpret_cast<PluginLibrary::FuncAddr>(functionAddress);
    }

    std::vector<std::string> PluginLibrary::ListFunctions()
    {
        // TODO: enumerate exported symbols of the ELF plugin (dl* API does not provide them)
        VEGA_CORE_WARN("PluginLibrary::ListFunctions is not implemented on this platform!");
        return {};
    }

}    // namespace Vega

#endif
