#pragma once

#include "Vega/ImGui/ImGuiImpl.hpp"
#include "Vega/Layers/GuiLayer.hpp"

#include <unordered_map>

struct ImFont;

namespace Vega
{

    class ImGuiLayer : public GuiLayer
    {
    public:
        ImGuiLayer();
        virtual ~ImGuiLayer() {};

        virtual void OnAttach(Ref<EventManager> _EventManager) override;
        virtual void OnDetach() override;

        virtual void OnUpdate() override;
        virtual void OnGuiRender() override;

        virtual void BeginGuiFrame() override;
        virtual void EndGuiFrame() override;

        void BlockEvents(bool block) { m_BlockEvents = block; }

        void SetDarkThemeColors();

        uint32_t GetActiveWidgetID() const;

        void LoadCustomFonts();

    private:
        Ref<ImGuiImpl> m_ImGuiImpl;

        bool m_BlockEvents = true;

        int m_FontSize = 14;
    };

}    // namespace Vega
