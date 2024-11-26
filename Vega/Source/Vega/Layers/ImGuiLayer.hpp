#pragma once

#include "Vega/Events/KeyEvent.hpp"
#include "Vega/Events/MouseEvent.hpp"
#include "Vega/Events/WindowEvent.hpp"
#include "Vega/ImGui/ImGuiImpl.hpp"
#include "Vega/Layers/Layer.hpp"

#include <unordered_map>

struct ImFont;

namespace Vega
{

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() = default;

        virtual void OnAttach(Ref<EventManager> _EventManager) override;
        virtual void OnDetach() override;

        virtual void OnUpdate() override;
        virtual void OnRender() override;

        void BlockEvents(bool block) { m_BlockEvents = block; }

        void SetDarkThemeColors();

        uint32_t GetActiveWidgetID() const;

        void ChangeFontSize(bool _NeedUpdateFontTexture);

        void SetFontSizeByMonitorScale(float _MonitorScale) { m_FontSize = static_cast<int>(14.0f * _MonitorScale); }

    private:
        Scope<ImGuiImpl> m_ImGuiImpl;

        bool m_BlockEvents = true;

        bool m_ChangeSize = false;
        int m_FontSize = 13;

        std::unordered_map<int, ImFont*> m_Fonts;
    };

}    // namespace Vega
