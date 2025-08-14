#include "Vega/Plugins/PluginLibrary.hpp"

#include "Vega/Core/Assert.hpp"

#include "Vega/Utils/PluginData.hpp"

#include "Platform/Platform.hpp"

#ifdef VEGA_PLATFORM_WINDOWS

namespace Vega
{

    void PluginLibrary::LoadPlugin()
    {
        std::filesystem::path pluginFilepath =
            std::filesystem::path(m_PluginLibraryProps.PluginPath) /
            std::filesystem::path(std::format("{}.dll", m_PluginLibraryProps.PluginName));

        m_LibraryHandle = LoadLibrary(pluginFilepath.string().c_str());

        if (!m_LibraryHandle)
        {
            VEGA_CORE_CRITICAL("Unable to load library: {}", pluginFilepath.string());
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
            FreeLibrary(m_LibraryHandle);
        }
    }

    PluginLibrary::FuncAddr PluginLibrary::LoadFunctionAddr(std::string_view _FunctionName)
    {
        FARPROC fAddrSetPluginGlobals = GetProcAddress(m_LibraryHandle, _FunctionName.data());
        return reinterpret_cast<PluginLibrary::FuncAddr>(fAddrSetPluginGlobals);
    }

    std::vector<std::string> PluginLibrary::ListFunctions()
    {
        PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(m_LibraryHandle);
        PIMAGE_NT_HEADERS ntHeaders =
            reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(m_LibraryHandle) + dosHeader->e_lfanew);
        DWORD exportDirRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        if (!exportDirRVA)
        {
            VEGA_CORE_WARN("No exported functions in plugin!");
            return {};
        }

        PIMAGE_EXPORT_DIRECTORY exportDir =
            (PIMAGE_EXPORT_DIRECTORY)(reinterpret_cast<BYTE*>(m_LibraryHandle) + exportDirRVA);
        DWORD* namesRVA =
            reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(m_LibraryHandle) + exportDir->AddressOfNames);
        DWORD count = exportDir->NumberOfNames;

        std::vector<std::string> result = std::vector<std::string>(count);
        for (DWORD i = 0; i < count; i++)
        {
            char* functionName = reinterpret_cast<char*>(reinterpret_cast<BYTE*>(m_LibraryHandle) + namesRVA[i]);
            result[i] = std::string(functionName);
        }

        return result;
    }

}    // namespace Vega

#endif
