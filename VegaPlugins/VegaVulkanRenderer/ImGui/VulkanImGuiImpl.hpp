#pragma once

#include "Vega/ImGui/ImGuiImpl.hpp"

#include <vulkan/vulkan.h>

namespace Vega
{

    class VulkanImGuiImpl : public ImGuiImpl
    {
    public:
        virtual void SetImGuiGlobalData(ImGuiContext* _Context, ImGuiMemAllocFunc* allocFunc,
                                        ImGuiMemFreeFunc* freeFunc, void** userData) override;

        virtual void Init() override;
        virtual void NewFrame() override;
        virtual void RenderFrame() override;
        virtual void Shutdown() override;
        virtual void RecreateFontTexture() override;
    };

}    // namespace Vega
