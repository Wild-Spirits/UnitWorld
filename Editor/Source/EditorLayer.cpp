#include "EditorLayer.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl"
#include "Vega/Renderer/RendererBackend.hpp"
#include "Vega/Renderer/Shader.hpp"
#include "glm/fwd.hpp"
#include "imgui.h"
#include "imgui_internal.h"

namespace Vega
{

    constexpr float kMainMenuFramePadding = 16.0f;

    void EditorLayer::OnAttach(Ref<EventManager> _EventManager)
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<Window> window = Application::Get().GetWindow();

        m_ViewportDimensions = { window->GetWidth(), window->GetHeight() };

        m_FrameBuffer = rendererBackend->CreateFrameBuffer({
            .Name = "TestFB",
            .Width = m_ViewportDimensions.x,
            .Height = m_ViewportDimensions.y,
            .IsUsedInFlight = true,
            .IsUsedForGui = true,
        });

        // m_FrameBuffer->Resize(m_ViewportDimensions.x, m_ViewportDimensions.y);

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

    void EditorLayer::OnDetach()
    {
        m_Shader->Shutdown();
        m_FrameBuffer->Destroy();
    }

    void EditorLayer::OnUpdate()
    {
        if (m_FrameBuffer->GetWidth() != m_ViewportDimensions.x || m_FrameBuffer->GetHeight() != m_ViewportDimensions.y)
        {
            m_FrameBuffer->Resize(m_ViewportDimensions.x, m_ViewportDimensions.y);
            // TODO: Try to make it better (without frame skipping)
            m_FramesToSkip = Application::Get().GetRendererBackend()->GetSwapchainColorTextures().size() * 2;
        }
    }

    void EditorLayer::OnRender()
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<Window> window = Application::Get().GetWindow();

        m_FrameBuffer->Bind();

        rendererBackend->BeginRendering({ 0.0f, 0.0f }, { m_FrameBuffer->GetWidth(), m_FrameBuffer->GetHeight() },
                                        m_FrameBuffer);

        rendererBackend->SetActiveViewport({ 0.0f, 0.0f }, { m_FrameBuffer->GetWidth(), m_FrameBuffer->GetHeight() });

        m_Shader->Bind();

        rendererBackend->TestFoo();

        rendererBackend->EndRendering();

        m_FrameBuffer->TransitToGui();
    }

    void EditorLayer::OnGuiRender()
    {
        float offsetY = DrawGuiTitlebar();

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImVec2 pos = viewport->Pos;
        pos.y += offsetY;
        ImVec2 size = viewport->Size;
        size.y -= offsetY;
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
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

        // if (ImGui::BeginMainMenuBar())
        // {
        //     if (ImGui::MenuItem("Test"))
        //     {
        //     }
        //     ImGui::EndMainMenuBar();
        // }

        if (ImGui::Begin("Scene"))
        {
        }
        ImGui::End();

        if (ImGui::Begin("Viewport", 0))
        {
            ImVec2 viewportDimensions = ImGui::GetContentRegionAvail();

            if (m_FramesToSkip == 0)
            {
                ImGui::Image((ImTextureID)m_FrameBuffer->GetInGuiRenderId(), viewportDimensions);
            }
            else
            {
                --m_FramesToSkip;
            }
            m_ViewportDimensions = {
                static_cast<uint32_t>(viewportDimensions.x),
                static_cast<uint32_t>(viewportDimensions.y),
            };
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

        if (m_IsDrawImGuiDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_IsDrawImGuiDemoWindow);
        }
    }

    bool EditorLayer::GuiDrawBeginMenu(std::string_view _Title)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.TouchExtraPadding.y = kMainMenuFramePadding;
        bool res = ImGui::BeginMenu(_Title.data());
        style.TouchExtraPadding.y = 0.0f;
        if (ImGui::IsItemHovered())
        {
            Application::Get().SetIsMainMenuAnyItemHovered(true);
        }

        return res;
    }

    bool EditorLayer::GuiDrawMenuButton(std::string_view _Title, float _CursorPosY, const glm::vec2& _Size)
    {
        ImGui::SetCursorPosY(_CursorPosY);
        bool res = ImGui::Button(_Title.data(), { _Size.x, _Size.y });
        if (ImGui::IsItemHovered())
        {
            Application::Get().SetIsMainMenuAnyItemHovered(true);
        }

        return res;
    }

    float EditorLayer::DrawGuiTitlebar()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float frameHeightOld = ImGui::GetFrameHeight();

        ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, kMainMenuFramePadding);
        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, kMainMenuFramePadding);
        ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, kMainMenuFramePadding);
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, 0xff1b1b1b);
        ImGuiWindowFlags viewportSideBarFlags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        Application::Get().SetIsMainMenuAnyItemHovered(false);
        float frameHeight = ImGui::GetFrameHeight();
        Application::Get().SetMainMenuFrameHeight(frameHeight);
        if (ImGui::BeginViewportSideBar("##Toolbar", viewport, ImGuiDir_Up, frameHeight, viewportSideBarFlags))
        {
            if (ImGui::BeginMenuBar())
            {
                ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, kMainMenuFramePadding);
                float logoHeight = frameHeight - kMainMenuFramePadding * 1.5f;
                float logoPosY = (frameHeight - logoHeight) / 2.0f;
                float cursorPosY = ImGui::GetCursorPosY();
                // ImGui::SetCursorPosY(cursorPosY + logoPosY);
                // ImGui::Image(reinterpret_cast<ImTextureID>(m_AppLogoLight->GetTextureId()),
                //              { m_AppLogoLight->GetWidth() / m_AppLogoLight->GetHeight() * logoHeight, logoHeight });

                ImGui::SetCursorPosY(cursorPosY);
                if (GuiDrawBeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
                    {
                        // OpenProject();
                    }

                    if (ImGui::MenuItem("New Project", "Ctrl+N"))
                    {
                        // NewProject();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                    {
                        // SaveProject();
                    }

                    if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S"))
                    {
                        // SaveProjectAs();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Close", "Ctrl+F4"))
                    {
                        // CloseProject();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Exit"))
                    {
                        Application::Get().Close();
                    }

                    ImGui::EndMenu();
                }

                ImGui::SetCursorPosY(cursorPosY);
                if (GuiDrawBeginMenu("Window"))
                {
                    // if (ImGui::MenuItem("Select Construction", nullptr, m_IsDrawSelectConstructionWindow))
                    // {
                    //     m_IsDrawSelectConstructionWindow = !m_IsDrawSelectConstructionWindow;
                    // }

                    // if (ImGui::MenuItem("Test Table", nullptr, m_IsDrawTestTableWindow))
                    // {
                    //     m_IsDrawTestTableWindow = !m_IsDrawTestTableWindow;
                    // }

                    // ImGui::Separator();

                    if (ImGui::MenuItem("ImGui Demo", nullptr, m_IsDrawImGuiDemoWindow))
                    {
                        m_IsDrawImGuiDemoWindow = !m_IsDrawImGuiDemoWindow;
                    }

                    ImGui::EndMenu();
                }

                ImGui::PopStyleVar();

                ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
                float buttonWidth = frameHeight * 1.2f;
                glm::vec2 buttonSize = { buttonWidth, frameHeight };
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth * 3.0f +
                                     ImGui::GetStyle().WindowPadding.x);

                Ref<Window> window = Application::Get().GetWindow();

                if (GuiDrawMenuButton(ICON_FA_MINUS, cursorPosY, buttonSize))
                {
                    window->Minimize();
                }

                if (window->IsWindowMaximized())
                {
                    if (GuiDrawMenuButton(ICON_FA_WINDOW_RESTORE, cursorPosY, buttonSize))
                    {
                        window->Restore();
                    }
                }
                else
                {
                    if (GuiDrawMenuButton(ICON_FA_WINDOW_MAXIMIZE, cursorPosY, buttonSize))
                    {
                        window->Maximize();
                    }
                }

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xff4f4eff);
                if (GuiDrawMenuButton(ICON_FA_XMARK, cursorPosY, buttonSize))
                {
                    Application::Get().Close();
                }
                ImGui::PopStyleColor();

                ImGui::PopStyleColor();

                ImGui::PopStyleVar();

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        return frameHeight - frameHeightOld;
    }

}    // namespace Vega
