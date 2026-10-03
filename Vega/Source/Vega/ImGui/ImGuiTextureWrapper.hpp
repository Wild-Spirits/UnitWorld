#pragma once

namespace Vega
{

    class ImGuiTextureWrapper
    {
    public:
        ImGuiTextureWrapper() = default;

        virtual ~ImGuiTextureWrapper() = default;

        virtual void OnDetach() = 0;

        virtual void* GetImGuiTextureId() const = 0;
    };

}    // namespace Vega
