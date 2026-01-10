#include "SceneHierarchyPanel.hpp"

#include "Vega/Scene/Scene.hpp"
#include "imgui.h"

#include "Vega/Scene/Components/HierarchyComponent.hpp"
#include "Vega/Scene/Components/NameComponent.hpp"
#include "Vega/Scene/Components/TransformComponent.hpp"

namespace Vega
{

    void SceneHierarchyPanel::OnImGuiRender(Ref<Scene> _Scene)
    {
        m_SceneContext = _Scene;

        if (ImGui::Button("TMP Remove Dirty Transoform component"))
        {
            m_SceneContext->ClearTransformDirtyFlags();
        }
        DrawHierarchy(_Scene->m_Registry);
    }

    void SceneHierarchyPanel::DrawHierarchy(entt::registry& _Registry)
    {
        _Registry.view<Components::HierarchyComponent>().each(
            [&](entt::entity entity, const Components::HierarchyComponent& hierarchy) {
                if (hierarchy.Parent == entt::null)
                {
                    DrawEntityNode(_Registry, entity);
                }
            });
    }

    void SceneHierarchyPanel::DrawEntityNode(entt::registry& _Registry, entt::entity _EntityId)
    {
        auto& hierarchy = _Registry.get<Components::HierarchyComponent>(_EntityId);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

        if (hierarchy.ChildCount == 0)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (_EntityId == m_SelectedEntity)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        Entity entity { _EntityId, m_SceneContext.get() };
        Components::NameComponent& nameComp = entity.GetComponent<Components::NameComponent>();

        bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(_EntityId)), flags, "%s - %s",
                                        nameComp.Name.c_str(),
                                        entity.HasComponent<Components::TransformDirtyComponent>() ? "Dirty" : "Clean");

        bool clicked = ImGui::IsItemClicked();
        if (clicked)
        {
            m_SelectedEntity = _EntityId;
        }
        // Если есть дети — рисуем их
        if (hierarchy.ChildCount > 0 && opened)
        {
            entt::entity child = hierarchy.FirstChild;
            while (child != entt::null)
            {
                DrawEntityNode(_Registry, child);
                child = _Registry.get<Components::HierarchyComponent>(child).NextSibling;
            }
            ImGui::TreePop();
        }
    }

}    // namespace Vega
