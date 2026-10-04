#include "SceneSystemStaticMeshDraw.hpp"
#include "Vega/Core/Application.hpp"

#include "Vega/Managers/StaticMeshManager.hpp"
#include "Vega/Renderer/Shader.hpp"
#include "Vega/Scene/Components/StaticMeshComponent.hpp"
#include "Vega/Scene/Scene.hpp"
#include <vector>

namespace Vega::SceneSystems
{

    SceneSystemStaticMeshDraw::SceneSystemStaticMeshDraw()
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();

        m_TestTexture = rendererBackend->CreateTexture("AppLogo", "Assets/Textures/logo_ws.png", TextureProps {});
        m_TestTextureSampler = rendererBackend->CreateSampler("AppLogoSampler", SamplerProps {});

        // NOTE: Packed into one UBO in declaration order, must match perFrameUbo in test.vert
        std::vector<ShaderUniform> perFrameUniforms = {
            ShaderUniform {
                           .Name = "view",
                           .Size = sizeof(glm::mat4),
                           .Type = ShaderUniformType::kMatrix4,
                           },
            ShaderUniform {
                           .Name = "proj",
                           .Size = sizeof(glm::mat4),
                           .Type = ShaderUniformType::kMatrix4,
                           }
        };
        std::vector<ShaderUniform> perGroupUniforms = {
            ShaderUniform {
                           .Name = "albedoTexture",
                           .Type = ShaderUniformType::kTexture2d,
                           },
            ShaderUniform {
                           .Name = "albedoSampler",
                           .Type = ShaderUniformType::kSampler2d,
                           }
        };
        m_Shader = Application::Get().GetRendererBackend()->CreateShader(
            ShaderConfig {
                .Name = "SceneSystemStaticMeshDraw",
                .Attributes = { ShaderAttributeType::kFloat3, ShaderAttributeType::kFloat2 },
                .UniformsPerFrame = perFrameUniforms,
                .UniformsPerGroup = perGroupUniforms,
                // Culling disabled: flipped viewport inverts winding, so the NDC-space quad would be culled as
                // back-face
                .CullMode = FaceCullMode::kNone,
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

    void SceneSystemStaticMeshDraw::Destroy()
    {
        m_Shader->OnDetach();
        m_TestTextureSampler->OnDetach();
        m_TestTexture->OnDetach();
    }

    void SceneSystemStaticMeshDraw::OnUpdate(Scene* _Scene, Timestep _Timestep) { }

    void SceneSystemStaticMeshDraw::OnRender(Scene* _Scene)
    {
        m_Shader->Bind();

        m_Shader->SetUniformBufferData("view", m_View, ShaderUpdateFrequency::kPerFrame);
        m_Shader->SetUniformBufferData("proj", m_Projection, ShaderUpdateFrequency::kPerFrame);
        m_Shader->ApplyFrequency(ShaderUpdateFrequency::kPerFrame);

        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<StaticMeshManager> staticMeshManager =
            StaticRefCast<StaticMeshManager>(Application::Get().GetManager("StaticMeshManager"));

        _Scene->GetRegistry().view<Components::StaticMeshComponent, Components::WorldTransformComponent>().each(
            [&](auto entity, const Components::StaticMeshComponent& meshComp,
                const Components::WorldTransformComponent& worldTransformComp) {
                // TODO: Continue implementation
                // Need to get StaticMeshManager from Application
                m_Shader->SetUniformTexture("albedoTexture", m_TestTexture, ShaderUpdateFrequency::kPerGroup);
                m_Shader->SetUniformSampler("albedoSampler", m_TestTextureSampler, ShaderUpdateFrequency::kPerGroup);
                m_Shader->ApplyFrequency(ShaderUpdateFrequency::kPerGroup);
                m_Shader->SetUniformBufferData("perDrawUbo.model", worldTransformComp.Matrix,
                                               ShaderUpdateFrequency::kPerDraw);
                StaticMeshManagerMeshInfo meshInfo = staticMeshManager->BindMesh(meshComp.MeshName);
                rendererBackend->DrawIndexed(static_cast<uint32_t>(meshInfo.IndexCount));
                // rendererBackend->TestFoo();

                // TODO: Actually we need to add objects to some render graph nodes
                // StaticMeshRenderNode::AddMesh(transform, mesh, material)
                // Store material descriptor sets in render grapth node
            });
    }

}    // namespace Vega::SceneSystems
