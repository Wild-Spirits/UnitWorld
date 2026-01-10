#include "EntityPropsPanel.hpp"

#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/Scene/Components/NameComponent.hpp"

#include "imgui.h"

namespace Vega
{

    void EntityPropsPanel::OnImGuiRender(Entity _Entity)
    {
        entt::registry& registry = _Entity.m_Scene->m_Registry;
        entt::entity entityId = _Entity.m_Handle;
        if (entityId == entt::null || !registry.valid(entityId))
        {
            ImGui::TextDisabled("No entity selected");
            return;
        }

        ImGui::TextUnformatted(_Entity.GetComponent<Components::NameComponent>().Name.c_str());

        ImGui::Button(" " ICON_FA_PLUS "  Add Component");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (auto&& [type, panelProps] : g_ComponentPropsPanels)
        {
            if (registry.storage(type) && registry.storage(type)->contains(entityId))
            {
                panelProps.DrawFunc(_Entity);
            }
        }
    }

}    // namespace Vega
