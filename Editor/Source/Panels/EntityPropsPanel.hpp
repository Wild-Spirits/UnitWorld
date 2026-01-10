#pragma once

#include "Vega/Scene/Scene.hpp"

#include "entt/core/fwd.hpp"

#include <unordered_map>

namespace Vega
{

    struct EntityPropsPanelComponentDescription
    {
        using PropsPanelFunc = void (*)(Entity);

        std::string Name;
        PropsPanelFunc DrawFunc;
    };

    class EntityPropsPanel
    {
    public:
        EntityPropsPanel() = default;
        ~EntityPropsPanel() = default;

        void OnImGuiRender(Entity _Entity);

        template <typename T>
        static void RegisterComponentDescription(const EntityPropsPanelComponentDescription& _Description)
        {
            entt::id_type typeHash = entt::type_hash<T>::value();
            if (g_ComponentPropsPanels.contains(typeHash))
            {
                return;
            }
            g_ComponentPropsPanels[typeHash] = _Description;
        }

    protected:
        static inline std::unordered_map<entt::id_type, EntityPropsPanelComponentDescription> g_ComponentPropsPanels;
    };

}    // namespace Vega
