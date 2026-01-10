#pragma once

#include "Components/TransformComponent.hpp"
#include "Systems/SceneSystem.hpp"
#include "Vega/Core/Assert.hpp"
#include <entt/entt.hpp>

#include <string_view>
#include <vector>

namespace Vega
{

    class Scene;

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity _Handle, Scene* _Scene) : m_Handle(_Handle), m_Scene(_Scene) { }
        Entity(const Entity& _Other) = default;
        ~Entity() = default;

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template <typename T>
        T& GetComponent();

        template <typename T>
        const T& GetConstComponent();

        // Специализация для запрета GetComponent<TransformComponent>
        template <>
        Components::TransformComponent& GetComponent<Components::TransformComponent>() = delete;

        const Components::TransformComponent& GetTransform();

        void SetTransform(const Components::TransformComponent& transform);
        void SetTransformPosition(const glm::vec3& position);
        void SetTransformRotation(const glm::quat& rotation);
        void SetTransformScale(const glm::vec3& scale);

        // TODO: Add SetTransform, SetTransformPosition, SetTransformRotation, SetTransformScale

        template <typename T>
        bool HasComponent();

        template <typename T>
        void RemoveComponent();

    protected:
        friend class Scene;
        friend class SceneHierarchyPanel;
        friend class EntityPropsPanel;

    protected:
        entt::entity m_Handle = entt::null;
        Scene* m_Scene = nullptr;
    };

    class Scene
    {
    public:
        Scene();
        virtual ~Scene();

        void OnUpdate(float _DeltaTime);

        Entity CreateEntity(std::string_view _Name, Entity _Parent = Entity());
        // TODO: CreateActor - like CreateEntity but with predefined components (e.g. Transform, etc.)
        Entity CreateActor(std::string_view _Name, Entity _Parent = Entity());
        void DestroyEntity(Entity _Entity);

        entt::registry& GetRegistry() { return m_Registry; }

        void OnUpdate();

        void OnRender();

        void AddSceneSystem(Ref<SceneSystems::SceneSystem> _SceneSystem) { m_SceneSystems.push_back(_SceneSystem); }

        // TMP:
        void ClearTransformDirtyFlags() { m_Registry.clear<Components::TransformDirtyComponent>(); }

    protected:
        friend class Entity;
        friend class SceneHierarchyPanel;
        friend class EntityPropsPanel;

    protected:
        entt::registry m_Registry;

        std::vector<Ref<SceneSystems::SceneSystem>> m_SceneSystems;
    };

    template <typename T, typename... Args>
    T& Entity::AddComponent(Args&&... args)
    {
        VEGA_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");

        return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template <typename T>
    T& Entity::GetComponent()
    {
        VEGA_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");

        return m_Scene->m_Registry.get<T>(m_Handle);
    }

    template <typename T>
    const T& Entity::GetConstComponent()
    {
        VEGA_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");

        return m_Scene->m_Registry.get<T>(m_Handle);
    }

    template <typename T>
    bool Entity::HasComponent()
    {
        return m_Scene->m_Registry.any_of<T>(m_Handle);
    }

    template <typename T>
    void Entity::RemoveComponent()
    {
        VEGA_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");

        m_Scene->m_Registry.remove<T>(m_Handle);
    }

}    // namespace Vega
