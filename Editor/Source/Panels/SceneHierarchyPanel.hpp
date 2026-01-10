#pragma once

#include "Vega/Scene/Scene.hpp"

namespace Vega
{

    class SceneHierarchyPanel
    {
    public:
        void OnImGuiRender(Ref<Scene> _Scene);

        Entity GetSelectedEntity() const { return Entity { m_SelectedEntity, m_SceneContext.get() }; }

    protected:
        void DrawHierarchy(entt::registry& _Registry);
        void DrawEntityNode(entt::registry& _Registry, entt::entity _Entity);

    protected:
        Ref<Scene> m_SceneContext;

        entt::entity m_SelectedEntity = entt::null;
    };

}    // namespace Vega
