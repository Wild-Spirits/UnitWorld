#pragma once

#include "SceneSystem.hpp"

#include "Vega/Renderer/Shader.hpp"

namespace Vega::SceneSystems
{

    class SceneSystemStaticMeshDraw : public SceneSystem
    {
    public:
        SceneSystemStaticMeshDraw();
        virtual ~SceneSystemStaticMeshDraw() = default;

        virtual void Destroy() override;

        virtual void OnUpdate(Scene* _Scene) override;

        virtual void OnRender(Scene* _Scene) override;

    protected:
        Ref<Texture> m_TestTexture;
        Ref<Sampler> m_TestTextureSampler;

        Ref<Shader> m_Shader;
    };

}    // namespace Vega::SceneSystems
