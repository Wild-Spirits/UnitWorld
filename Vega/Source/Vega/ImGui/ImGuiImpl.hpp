#pragma once

#include "Vega/Core/Base.hpp"

struct ImGuiContext;
typedef void* (*ImGuiMemAllocFunc)(size_t sz, void* user_data);
typedef void (*ImGuiMemFreeFunc)(void* ptr, void* user_data);

namespace Vega
{

    class ImGuiImpl
    {
    public:
        virtual void SetImGuiGlobalData(ImGuiContext* _Context, ImGuiMemAllocFunc* allocFunc,
                                        ImGuiMemFreeFunc* freeFunc, void** userData) = 0;

        virtual void Init() = 0;
        virtual void NewFrame() = 0;
        virtual void RenderFrame() = 0;
        virtual void Shutdown() = 0;

        virtual void RecreateFontTexture() = 0;

        virtual void BackupCurrentWindowContext() {};
        virtual void RestoreCurrentWindowContext() {};
    };

}    // namespace Vega
