#include "SceneSystemStaticMeshDraw.hpp"
#include "Vega/Core/Application.hpp"

#include "Vega/Managers/StaticMeshManager.hpp"
#include "Vega/Scene/Components/StaticMeshComponent.hpp"
#include "Vega/Scene/Scene.hpp"

namespace Vega::SceneSystems
{

    SceneSystemStaticMeshDraw::SceneSystemStaticMeshDraw()
    {
        m_Shader = Application::Get().GetRendererBackend()->CreateShader(
            ShaderConfig {
                .Name = "EditorLayerTestShader",
                .Attributes = { ShaderAttributeType::kFloat3 },
        },
            { ShaderStageConfig {
                  .Type = ShaderStageConfig::ShaderStageType::kVertex,
                  .Path = "Assets/Shaders/Source/test.vert",
              },
              ShaderStageConfig {
                  .Type = ShaderStageConfig::ShaderStageType::kFragment,
                  .Path = "Assets/Shaders/Source/test.frag",
              } });
    }

    void SceneSystemStaticMeshDraw::Destroy() { m_Shader->Shutdown(); }

    void SceneSystemStaticMeshDraw::OnUpdate(Scene* _Scene) { }

    void SceneSystemStaticMeshDraw::OnRender(Scene* _Scene)
    {
        m_Shader->Bind();

        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<StaticMeshManager> staticMeshManager =
            StaticRefCast<StaticMeshManager>(Application::Get().GetManager("StaticMeshManager"));

        _Scene->GetRegistry().view<Components::StaticMeshComponent, Components::TransformComponent>().each(
            [&](auto entity, const Components::StaticMeshComponent& meshComp,
                const Components::TransformComponent& transformComp) {
                // TODO: Continue implementation
                // Need to get StaticMeshManager from Application
                m_Shader->SetUniformBufferData("perDrawUbo.model", transformComp.GetTransformMatrix(),
                                               ShaderUpdateFrequency::kPerDraw);
                staticMeshManager->BindMesh(meshComp.MeshName);
                rendererBackend->TestFoo();
            });
    }

}    // namespace Vega::SceneSystems
