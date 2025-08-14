#pragma once

#include "Platform/Platform.hpp"

#include <filesystem>

#ifdef VEGA_PLATFORM_WINDOWS
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

namespace Vega
{

    struct PluginLibraryProps
    {
        std::string PluginName;
        std::filesystem::path PluginPath;
        bool IsNeedSetPluginGlobals = false;
    };

    class PluginLibrary
    {
    private:
        typedef void* (*FuncAddr)();

    public:
        PluginLibrary(const PluginLibraryProps& _PluginLibraryProps);
        ~PluginLibrary();

        void LoadPlugin();
        void UnloadPlugin();

        void ReloadPlugin();

        template <typename Func>
        Func LoadFunction(std::string_view _FunctionName)
        {
            return reinterpret_cast<Func>(LoadFunctionAddr(_FunctionName));
        }

        static std::filesystem::path GetDefaultPluginPath();

        std::vector<std::string> ListFunctions();

    protected:
        FuncAddr LoadFunctionAddr(std::string_view _FunctionName);

    protected:
#ifdef VEGA_PLATFORM_WINDOWS
        HMODULE m_LibraryHandle = nullptr;
#endif

        PluginLibraryProps m_PluginLibraryProps;
    };

}    // namespace Vega
