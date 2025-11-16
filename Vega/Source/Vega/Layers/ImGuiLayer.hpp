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

        void ChangeFontSize(bool _NeedUpdateFontTexture);

        void SetFontSize(int _FontSize = 14) { m_FontSize = _FontSize; }

    private:
        Ref<ImGuiImpl> m_ImGuiImpl;

        bool m_BlockEvents = true;

        bool m_ChangeSize = false;
        int m_FontSize = 14;

        std::unordered_map<int, ImFont*> m_Fonts;
    };

}    // namespace Vega
