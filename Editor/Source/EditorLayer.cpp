#include "EditorLayer.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl"
#include "Vega/Renderer/RendererBackend.hpp"
#include "Vega/Renderer/Shader.hpp"
#include "Vega/Renderer/Texture.hpp"
#include "imgui.h"
#include "imgui_internal.h"

namespace Vega
{

    void EditorLayer::OnAttach(Ref<EventManager> _EventManager)
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<Window> window = Application::Get().GetWindow();

        // size_t imagesCount = rendererBackend->GetSwapchainColorTextures().size();

        // for (size_t i = 0; i < imagesCount; ++i)
        // {
        //     Ref<Texture> texture =
        //         rendererBackend->CreateTexture(std::format("TestImg_{}", i), {
        //                                                                          .Width = window->GetWidth(),
        //                                                                          .Height = window->GetHeight(),
        //                                                                          .ChannelCount = 4,
        //                                                                      });

        //     m_ColorBuffers.push_back(texture);
        // }

        m_FrameBuffer =
            rendererBackend->CreateFrameBuffer({ .Name = "TestFB", .IsUsedInFlight = true, .IsUsedForGui = true });

        m_Shader = Application::Get().GetRendererBackend()->CreateShader(
            ShaderConfig {
                .Name = "EditorLayerTestShader",
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

    void EditorLayer::OnDetach() { m_Shader->Shutdown(); }

    void EditorLayer::OnUpdate() { }

    void EditorLayer::OnRender()
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<Window> window = Application::Get().GetWindow();

        m_FrameBuffer->Bind();
        // 1. Begin rendering
        rendererBackend->BeginRendering({ 0.0f, 0.0f }, { window->GetWidth(), window->GetHeight() },
                                        { m_FrameBuffer->GetTextures() }, std::vector<std::vector<Ref<Texture>>>());

        rendererBackend->SetActiveViewport({ 0.0f, 0.0f }, { window->GetWidth(), window->GetHeight() });

        // 2. Shader
        m_Shader->Bind();

        rendererBackend->TestFoo();

        // 3. End Rendering
        rendererBackend->EndRendering();

        m_FrameBuffer->TransitToGui();
    }

    void EditorLayer::OnGuiRender()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Window Main", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        // ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
        ImGuiID dockspaceId = ImGui::GetID("DockSpace Main");
        ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

        static auto firstTime = true;
        if (firstTime)
        {
            firstTime = false;

            ImGui::DockBuilderRemoveNode(dockspaceId);    // clear any previous layout
            ImGui::DockBuilderAddNode(dockspaceId,
                                      ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

            auto dockIdRight = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, nullptr, &dockspaceId);
            auto dockIdBottom = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.2f, nullptr, &dockspaceId);

            ImGui::DockBuilderDockWindow("Scene", dockIdRight);
            ImGui::DockBuilderDockWindow("Viewport", dockspaceId);
            ImGui::DockBuilderDockWindow("Assets", dockIdBottom);
            ImGui::DockBuilderFinish(dockspaceId);
        }

        ImGui::End();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("Test"))
            {
            }
            ImGui::EndMainMenuBar();
        }

        if (ImGui::Begin("Scene"))
        {
        }
        ImGui::End();

        if (ImGui::Begin("Viewport", 0))
        {
            ImGui::Image((ImTextureID)m_FrameBuffer->GetInGuiRenderId(), { 800.0f, 600.0f });
        }
        ImGui::End();

        if (ImGui::Begin("Assets", 0))
        {
            ImGui::Text("Hello, world!");
            ImGui::Text("Привет Мир!");

            if (ImGui::Button(ICON_FA_FILE_WORD ICON_FA_BRIDGE ICON_FA_LINUX "Button"))
            {
            }

            static char testText[256] = "";
            ImGui::InputText("TestText", testText, 256);
        }
        ImGui::End();

        if (ImGui::Begin("Props"))
        {
        }
        ImGui::End();

        ImGui::ShowDemoWindow();
    }

}    // namespace Vega
