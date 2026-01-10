#include "Scene.hpp"

#include "Components/HierarchyComponent.hpp"
#include "Components/NameComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Vega/Core/Assert.hpp"

#include "entt/entity/fwd.hpp"

#include <stack>

namespace Vega
{

    void OnTransformConstructOrUpdate(entt::registry& _Registry, entt::entity _Entity)
    {
        VEGA_CORE_ASSERT(_Entity != entt::null, "Invalid entity!");

        if (_Registry.any_of<Components::TransformDirtyComponent>(_Entity) ||
            !_Registry.any_of<Components::TransformComponent>(_Entity))
        {
            return;
        }

        std::stack<entt::entity> entitiesToProcess;
        entitiesToProcess.push(_Entity);

        while (!entitiesToProcess.empty())
        {
            entt::entity currentEntity = entitiesToProcess.top();
            entitiesToProcess.pop();

            _Registry.emplace<Components::TransformDirtyComponent>(currentEntity);

            entt::entity childEntity = _Registry.get<Components::HierarchyComponent>(currentEntity).FirstChild;
            while (childEntity != entt::null)
            {
                entt::entity currentChildEntity = childEntity;
                childEntity = _Registry.get<Components::HierarchyComponent>(childEntity).NextSibling;

                if (_Registry.any_of<Components::TransformDirtyComponent>(currentChildEntity) ||
                    !_Registry.any_of<Components::TransformComponent>(currentChildEntity))
                {
                    continue;
                }

                entitiesToProcess.push(currentChildEntity);
            }
        }
    }

    const Components::TransformComponent& Entity::GetTransform()
    {
        VEGA_CORE_ASSERT(HasComponent<Components::TransformComponent>(), "Entity does not have Transform component!");
        return GetConstComponent<Components::TransformComponent>();
    }

    void Entity::SetTransform(const Components::TransformComponent& transform)
    {
        VEGA_CORE_ASSERT(HasComponent<Components::TransformComponent>(), "Entity does not have Transform component!");
        m_Scene->m_Registry.replace<Components::TransformComponent>(m_Handle, transform);
        OnTransformConstructOrUpdate(m_Scene->m_Registry, m_Handle);
    }

    void Entity::SetTransformPosition(const glm::vec3& position)
    {
        VEGA_CORE_ASSERT(HasComponent<Components::TransformComponent>(), "Entity does not have Transform component!");
        Components::TransformComponent& transformComponent =
            m_Scene->m_Registry.get<Components::TransformComponent>(m_Handle);
        transformComponent.Position = position;
        OnTransformConstructOrUpdate(m_Scene->m_Registry, m_Handle);
    }

    void Entity::SetTransformRotation(const glm::quat& rotation)
    {
        VEGA_CORE_ASSERT(HasComponent<Components::TransformComponent>(), "Entity does not have Transform component!");
        Components::TransformComponent& transformComponent =
            m_Scene->m_Registry.get<Components::TransformComponent>(m_Handle);
        transformComponent.Rotation = rotation;
        OnTransformConstructOrUpdate(m_Scene->m_Registry, m_Handle);
    }

    void Entity::SetTransformScale(const glm::vec3& scale)
    {
        VEGA_CORE_ASSERT(HasComponent<Components::TransformComponent>(), "Entity does not have Transform component!");
        Components::TransformComponent& transformComponent =
            m_Scene->m_Registry.get<Components::TransformComponent>(m_Handle);
        transformComponent.Scale = scale;
        OnTransformConstructOrUpdate(m_Scene->m_Registry, m_Handle);
    }

    Scene::Scene()
    {
        m_Registry.on_construct<Components::TransformComponent>().connect<OnTransformConstructOrUpdate>();
    }

    Scene::~Scene()
    {
        for (auto& sceneSystem : m_SceneSystems)
        {
            sceneSystem->Destroy();
        }
        m_SceneSystems.clear();
    }

    void Scene::OnUpdate() { }

    void Scene::OnRender()
    {
        for (auto& sceneSystem : m_SceneSystems)
        {
            sceneSystem->OnRender(this);
        }
    }

    Entity Scene::CreateEntity(std::string_view _Name, Entity _Parent)
    {
        Entity entity { m_Registry.create(), this };
        entity.AddComponent<Components::NameComponent>(_Name);

        entt::entity nextSibling = entt::null;
        if (_Parent.m_Handle != entt::null)
        {
            Components::HierarchyComponent& parentHierarchyComp =
                _Parent.GetComponent<Components::HierarchyComponent>();
            nextSibling = parentHierarchyComp.FirstChild;
            parentHierarchyComp.FirstChild = entity.m_Handle;
            parentHierarchyComp.ChildCount++;

            if (nextSibling != entt::null)
            {
                Components::HierarchyComponent& nextSiblingHierarchyComp =
                    Entity { nextSibling, this }.GetComponent<Components::HierarchyComponent>();
                nextSiblingHierarchyComp.PrevSibling = entity.m_Handle;
            }
        }
        entity.AddComponent<Components::HierarchyComponent>(_Parent.m_Handle, nextSibling);

        return entity;
    }

    Entity Scene::CreateActor(std::string_view _Name, Entity _Parent)
    {
        // TODO: Check parent has transform or parent is entt::null. Otherwise throw error

        Entity entity = CreateEntity(_Name, _Parent);
        entity.AddComponent<Components::TransformComponent>();
        return entity;
    }

    void Scene::DestroyEntity(Entity _Entity)
    {
        // TODO: Implement proper entity destruction with hierarchy updates
    }

}    // namespace Vega
