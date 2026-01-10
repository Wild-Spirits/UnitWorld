#pragma once

#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"
#include <cstddef>

namespace Vega::Components
{

    // TODO: May add Root HierarchyComponent to represent root nodes in the hierarchy
    // (for performance of SceneHierarchyPanel)

    struct HierarchyComponent
    {
        HierarchyComponent() = default;
        HierarchyComponent(const HierarchyComponent&) = default;
        HierarchyComponent(entt::entity _Parent, entt::entity _NextSibling = entt::null)
            : Parent(_Parent),
              NextSibling(_NextSibling)
        { }

        size_t ChildCount { 0 };
        entt::entity FirstChild { entt::null };
        entt::entity Parent { entt::null };
        entt::entity PrevSibling { entt::null };
        entt::entity NextSibling { entt::null };
    };

}    // namespace Vega::Components
