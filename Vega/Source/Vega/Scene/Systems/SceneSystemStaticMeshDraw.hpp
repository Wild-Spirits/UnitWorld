#pragma once

#include "SceneSystem.hpp"

#include "Vega/Renderer/Shader.hpp"

#include <glm/glm.hpp>

namespace Vega::SceneSystems
{

    class SceneSystemStaticMeshDraw : public SceneSystem
    {
    public:
        SceneSystemStaticMeshDraw();
        virtual ~SceneSystemStaticMeshDraw() = default;

        virtual void Destroy() override;

        virtual void OnUpdate(Scene* _Scene, Timestep _Timestep) override;

        virtual void OnRender(Scene* _Scene) override;

        // TODO: Tmp until camera component: matrices are set from outside (EditorLayer test camera)
        void SetViewProjection(const glm::mat4& _View, const glm::mat4& _Projection)
        {
            m_View = _View;
            m_Projection = _Projection;
        }

    protected:
        glm::mat4 m_View { 1.0f };
        glm::mat4 m_Projection { 1.0f };

        Ref<Texture> m_TestTexture;
        Ref<Sampler> m_TestTextureSampler;

        Ref<Shader> m_Shader;
    };

}    // namespace Vega::SceneSystems
