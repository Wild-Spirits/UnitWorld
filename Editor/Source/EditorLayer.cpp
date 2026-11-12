#include "EditorLayer.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl"
#include "Vega/ImGui/ImGuiViewportTitleBar.hpp"
#include "Vega/Managers/MaterialManager.hpp"
#include "Vega/Managers/StaticMeshManager.hpp"
#include "Vega/Managers/TextureManager.hpp"
#include "Vega/Renderer/RendererBackend.hpp"
#include "Vega/Renderer/Texture.hpp"
#include "Vega/Scene/Components/CameraComponent.hpp"
#include "Vega/Scene/Components/StaticMeshComponent.hpp"
#include "Vega/Scene/Components/TransformComponent.hpp"
#include "Vega/Scene/Scene.hpp"
#include "Vega/Scene/Systems/SceneSystemStaticMeshDraw.hpp"

#include "glm/fwd.hpp"
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace Vega
{

    // TODO: Move to proper place
    void DrawTransform(Entity _Entity)
    {
        const Components::TransformComponent& transformComp = _Entity.GetTransform();

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            glm::vec3 newPosition = transformComp.Position;
            if (ImGui::DragFloat3("Position", &newPosition.x, 0.01f))
            {
                _Entity.SetTransformPosition(newPosition);
            }

            glm::vec3 rotationEulerDeg = glm::degrees(glm::eulerAngles(transformComp.Rotation));
            if (ImGui::DragFloat3("Rotation", &rotationEulerDeg.x, 0.01f))
            {
                glm::quat newRotation = glm::quat(glm::radians(rotationEulerDeg));
                _Entity.SetTransformRotation(newRotation);
            }

            glm::vec3 newScale = transformComp.Scale;
            if (ImGui::DragFloat3("Scale", &newScale.x, 0.1f))
            {
                _Entity.SetTransformScale(newScale);
            }
        }
    }

    // TODO: Move to proper place
    void DrawCamera(Entity _Entity)
    {
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Components::CameraComponent& cameraComp = _Entity.GetComponent<Components::CameraComponent>();

            bool isPrimary = cameraComp.IsPrimary;
            if (ImGui::Checkbox("Primary", &isPrimary))
            {
                if (isPrimary)
                {
                    // Only one primary camera in the scene
                    _Entity.GetScene()->SetPrimaryCamera(_Entity);
                }
                else
                {
                    cameraComp.IsPrimary = false;
                }
            }

            DrawGuiCameraProjection(cameraComp.Projection);
        }
    }

    constexpr float kMainMenuFramePadding = 16.0f;

    namespace
    {

        // Detached panels are resized by the window manager (viewport title bar) and must not collapse on
        // caption double click (it maximizes instead)
        ImGuiWindowFlags GetPanelFlags(bool _IsDetached)
        {
            return _IsDetached ? (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : ImGuiWindowFlags_None;
        }

    }    // namespace

    void EditorLayer::OnAttach(Ref<EventManager> _EventManager)
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<Window> window = Application::Get().GetWindow();

        m_ViewportDimensions = { window->GetWidth(), window->GetHeight() };

        m_FrameBuffer = rendererBackend->CreateFrameBuffer(FrameBufferProps {
            .Name = "GameFrameBuffer",
            .Width = m_ViewportDimensions.x,
            .Height = m_ViewportDimensions.y,
            .IsUsedInFlight = true,
        });
        m_FrameBufferSampler = rendererBackend->CreateSampler("GameFrameBufferSampler", SamplerProps {});
        m_FrameBufferImGuiTexture = rendererBackend->CreateImGuiFrameBufferWrapper(m_FrameBuffer, m_FrameBufferSampler);

        m_AppLogo = rendererBackend->CreateTexture("AppLogo", "Assets/Textures/logo_ws.png", TextureProps {});
        m_AppLogoSampler = rendererBackend->CreateSampler("AppLogoSampler", SamplerProps {});
        m_AppLogoImGuiTexture = rendererBackend->CreateImGuiTextureWrapper(m_AppLogo, m_AppLogoSampler);

        // m_FrameBuffer->Resize(m_ViewportDimensions.x, m_ViewportDimensions.y);

        Ref<StaticMeshManager> staticMeshManager = CreateRef<StaticMeshManager>();
        Application::Get().AddManager("StaticMeshManager", staticMeshManager);

        Ref<TextureManager> textureManager = CreateRef<TextureManager>();
        Application::Get().AddManager("TextureManager", textureManager);

        Ref<MaterialManager> materialManager = CreateRef<MaterialManager>();
        Application::Get().AddManager("MaterialManager", materialManager);

        std::vector<StaticMeshVertex> vertices {
            { .Position = { -0.5f, -0.5f, 1.0f }, .TexCoord = { 0.0f, 0.0f } },
            {  .Position = { 0.5f, -0.5f, 1.0f }, .TexCoord = { 1.0f, 0.0f } },
            {   .Position = { 0.5f, 0.5f, 1.0f }, .TexCoord = { 1.0f, 1.0f } },
            {  .Position = { -0.5f, 0.5f, 1.0f }, .TexCoord = { 0.0f, 1.0f } },
        };

        std::vector<uint32_t> indices { 0, 1, 2, 2, 3, 0 };

        staticMeshManager->AddMesh("TestMesh", vertices.data(), vertices.size(), indices.data(), indices.size(), false);

        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->AddSceneSystem(CreateRef<SceneSystems::SceneSystemStaticMeshDraw>());

        // Game camera, used in Play mode
        Entity mainCamera = m_ActiveScene->CreateActor("MainCamera");
        mainCamera.SetTransformPosition({ 0.0f, 0.0f, 3.0f });
        mainCamera.AddComponent<Components::CameraComponent>().IsPrimary = true;

        m_ActiveScene->CreateEntity("Test1");
        m_ActiveScene->CreateEntity("Test2");
        Entity Entity3 = m_ActiveScene->CreateActor("Test3");
        Entity Entity31 = m_ActiveScene->CreateActor("Test31", Entity3);
        Entity Entity32 = m_ActiveScene->CreateActor("Test32", Entity3);
        Entity Entity311 = m_ActiveScene->CreateActor("Test311", Entity31);
        Entity Entity312 = m_ActiveScene->CreateActor("Test312", Entity31);
        Entity Entity313 = m_ActiveScene->CreateActor("Test313", Entity31);
        Entity Entity314 = m_ActiveScene->CreateActor("Test314", Entity31);
        Entity311.AddComponent<Components::StaticMeshComponent>("TestMesh");
        // Entity312.AddComponent<Components::StaticMeshComponent>("TestMesh");
        // Entity313.AddComponent<Components::StaticMeshComponent>("TestMesh");
        // Entity314.AddComponent<Components::StaticMeshComponent>("TestMesh");

        EntityPropsPanel::RegisterComponentDescription<Components::TransformComponent>(
            EntityPropsPanelComponentDescription {
                .Name = "Transform",
                .DrawFunc = DrawTransform,
            });
        EntityPropsPanel::RegisterComponentDescription<Components::CameraComponent>(
            EntityPropsPanelComponentDescription {
                .Name = "Camera",
                .DrawFunc = DrawCamera,
            });
    }

    void EditorLayer::OnDetach()
    {
        m_ActiveScene.reset();

        m_AppLogoImGuiTexture->OnDetach();
        m_AppLogoSampler->OnDetach();
        m_AppLogo->OnDetach();

        m_FrameBufferImGuiTexture->OnDetach();
        m_FrameBufferSampler->OnDetach();
        m_FrameBuffer->Destroy();
    }

    void EditorLayer::OnUpdate(Timestep _Timestep)
    {
        m_LastTimestep = _Timestep;

        ResizeViewportFrameBuffer();

        m_ActiveScene->OnUpdate(_Timestep);
    }

    void EditorLayer::ResizeViewportFrameBuffer()
    {
        if (m_ViewportDimensions.x == 0 || m_ViewportDimensions.y == 0 ||
            (m_FrameBuffer->GetWidth() == m_ViewportDimensions.x &&
             m_FrameBuffer->GetHeight() == m_ViewportDimensions.y))
        {
            return;
        }

        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();

        // NOTE: Called before the frame is recorded. Old textures and ImGui descriptor sets may still be used by frames
        // in flight, but the backend destroys them only after those frames are complete, so no GPU wait is needed
        m_FrameBufferImGuiTexture->OnDetach();
        m_FrameBuffer->Resize(m_ViewportDimensions.x, m_ViewportDimensions.y);
        m_FrameBufferImGuiTexture = rendererBackend->CreateImGuiFrameBufferWrapper(m_FrameBuffer, m_FrameBufferSampler);
    }

    RenderView EditorLayer::GetActiveRenderView()
    {
        glm::uvec2 viewportSize = { m_FrameBuffer->GetWidth(), m_FrameBuffer->GetHeight() };

        if (m_SceneState == SceneState::kPlay)
        {
            std::optional<RenderView> gameView = m_ActiveScene->GetPrimaryCameraView(viewportSize);
            if (gameView.has_value())
            {
                return gameView.value();
            }
            // No primary camera: stay on the editor camera, the viewport shows a warning
        }

        return m_EditorCamera.GetRenderView(viewportSize);
    }

    void EditorLayer::OnRender()
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();
        Ref<Window> window = Application::Get().GetWindow();

        // Ensure the framebuffer is writable as attachment for the scene pass.
        // m_FrameBuffer->Bind();
        m_FrameBuffer->BindAndClearColorDepthStencil();

        rendererBackend->BeginRendering({ 0.0f, 0.0f }, { m_FrameBuffer->GetWidth(), m_FrameBuffer->GetHeight() },
                                        m_FrameBuffer);

        rendererBackend->SetActiveViewport({ 0.0f, 0.0f }, { m_FrameBuffer->GetWidth(), m_FrameBuffer->GetHeight() });

        m_ActiveScene->OnRender(GetActiveRenderView());

        rendererBackend->EndRendering();

        m_FrameBuffer->TransitColorAttachmentToGui();
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
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_PassthruCentralNode |
                                                       static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_DockSpace));
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

            auto dockIdLeft = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, nullptr, &dockspaceId);
            auto dockIdBottom = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.2f, nullptr, &dockspaceId);
            auto dockIdRight = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.2f, nullptr, &dockspaceId);

            ImGui::DockBuilderDockWindow("Props", dockIdLeft);
            ImGui::DockBuilderDockWindow("Editor Camera", dockIdLeft);
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

        if (m_IsDrawScenePanel)
        {
            const bool isOpen = ImGui::Begin("Scene", nullptr, GetPanelFlags(m_IsScenePanelDetached));
            DrawViewportTitleBarButtons(&m_IsDrawScenePanel, &m_IsScenePanelDetached);
            if (isOpen)
            {
                m_SceneHierarchyPanel.OnImGuiRender(m_ActiveScene);
            }
            ImGui::End();
        }

        if (m_IsDrawViewportPanel)
        {
            const bool isOpen = ImGui::Begin("Viewport", nullptr, GetPanelFlags(m_IsViewportPanelDetached));
            DrawViewportTitleBarButtons(&m_IsDrawViewportPanel, &m_IsViewportPanelDetached);
            if (isOpen)
            {
                DrawGuiViewport();
            }
            ImGui::End();
        }

        if (m_IsDrawAssetsPanel)
        {
            const bool isOpen = ImGui::Begin("Assets", nullptr, GetPanelFlags(m_IsAssetsPanelDetached));
            DrawViewportTitleBarButtons(&m_IsDrawAssetsPanel, &m_IsAssetsPanelDetached);
            if (isOpen)
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
        }

        if (m_IsDrawPropsPanel)
        {
            const bool isOpen = ImGui::Begin("Props", nullptr, GetPanelFlags(m_IsPropsPanelDetached));
            DrawViewportTitleBarButtons(&m_IsDrawPropsPanel, &m_IsPropsPanelDetached);
            if (isOpen)
            {
                m_EntityPropsPanel.OnImGuiRender(m_SceneHierarchyPanel.GetSelectedEntity());
            }
            ImGui::End();
        }

        if (m_IsDrawEditorCameraPanel)
        {
            const bool isOpen = ImGui::Begin("Editor Camera", nullptr, GetPanelFlags(m_IsEditorCameraPanelDetached));
            DrawViewportTitleBarButtons(&m_IsDrawEditorCameraPanel, &m_IsEditorCameraPanelDetached);
            if (isOpen)
            {
                m_EditorCamera.DrawGuiSettings();
            }
            ImGui::End();
        }

        if (m_IsDrawImGuiDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_IsDrawImGuiDemoWindow);
        }
    }

    void EditorLayer::DrawGuiViewport()
    {
        ImVec2 viewportDimensions = ImGui::GetContentRegionAvail();
        m_ViewportDimensions = {
            static_cast<uint32_t>(std::max(viewportDimensions.x, 0.0f)),
            static_cast<uint32_t>(std::max(viewportDimensions.y, 0.0f)),
        };
        if (m_ViewportDimensions.x == 0 || m_ViewportDimensions.y == 0)
        {
            return;
        }

        ImVec2 imagePos = ImGui::GetCursorPos();
        // NOTE: The framebuffer is resized in the next OnUpdate, so the image may be stretched for one frame
        ImGui::Image((ImTextureID)m_FrameBufferImGuiTexture->GetImGuiColorAttachmentId(), viewportDimensions);

        // Invisible button over the image takes the mouse, so dragging in the viewport doesn't move the window
        ImGui::SetCursorPos(imagePos);
        ImGui::InvisibleButton("##ViewportInput", viewportDimensions,
                               ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                                   ImGuiButtonFlags_MouseButtonMiddle);
        bool isViewportHovered = ImGui::IsItemHovered();

        if (m_SceneState == SceneState::kEdit)
        {
            m_EditorCamera.OnUpdate(m_LastTimestep, isViewportHovered);
        }
        else if (!m_ActiveScene->GetPrimaryCameraEntity().IsValid())
        {
            ImGui::SetCursorPos({ imagePos.x + 8.0f, imagePos.y + 8.0f });
            ImGui::TextColored({ 1.0f, 0.6f, 0.2f, 1.0f }, "No primary camera in the scene, using the editor camera");
        }
    }

    void EditorLayer::DrawGuiPlayStopButton(float _FrameHeight, float _CursorPosY)
    {
        bool isPlaying = m_SceneState == SceneState::kPlay;

        ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
        float buttonWidth = _FrameHeight * 1.2f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);
        if (GuiDrawMenuButton(isPlaying ? ICON_FA_STOP : ICON_FA_PLAY, _CursorPosY, { buttonWidth, _FrameHeight }))
        {
            // TODO: Copy the scene on Play and restore it on Stop, run scene logic only while playing
            m_SceneState = isPlaying ? SceneState::kEdit : SceneState::kPlay;
        }
        ImGui::PopStyleColor();
    }

    bool EditorLayer::GuiDrawBeginMenu(std::string_view _Title)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.TouchExtraPadding.y = kMainMenuFramePadding;
        bool res = ImGui::BeginMenu(_Title.data());
        style.TouchExtraPadding.y = 0.0f;
        // The menu reacts to the mouse in the extra padding too
        AddTitleBarInteractiveItem({ 0.0f, kMainMenuFramePadding });

        return res;
    }

    bool EditorLayer::GuiDrawMenuButton(std::string_view _Title, float _CursorPosY, const glm::vec2& _Size)
    {
        ImGui::SetCursorPosY(_CursorPosY);
        bool res = ImGui::Button(_Title.data(), { _Size.x, _Size.y });
        AddTitleBarInteractiveItem();

        return res;
    }

    void EditorLayer::AddTitleBarInteractiveItem(const glm::vec2& _ExtraPadding)
    {
        // Window coordinates are relative to the main viewport (with multi-viewports ImGui uses screen ones)
        ImVec2 viewportPos = ImGui::GetMainViewport()->Pos;
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();

        m_TitleBarLayout.InteractiveItems.push_back({
            .Min = { itemMin.x - viewportPos.x - _ExtraPadding.x, itemMin.y - viewportPos.y - _ExtraPadding.y },
            .Max = { itemMax.x - viewportPos.x + _ExtraPadding.x, itemMax.y - viewportPos.y + _ExtraPadding.y },
        });
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
        float frameHeight = ImGui::GetFrameHeight();

        Ref<Window> window = Application::Get().GetWindow();
        bool isCustomTitleBar = window->IsCustomTitleBar();
        m_TitleBarLayout = { .Height = frameHeight };

        if (ImGui::BeginViewportSideBar(std::format("##Toolbar{}", reinterpret_cast<void*>(viewport)).c_str(), viewport,
                                        ImGuiDir_Up, frameHeight, viewportSideBarFlags))
        {
            if (ImGui::BeginMenuBar())
            {
                ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, kMainMenuFramePadding);
                if (isCustomTitleBar && window->IsTitleBarHasNativeButtons())
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + window->GetTitleBarNativeButtonsWidth());
                }

                float logoHeight = frameHeight - kMainMenuFramePadding * 0.5f;
                float logoPosY = (frameHeight - logoHeight) / 2.0f;
                float cursorPosY = ImGui::GetCursorPosY();
                ImGui::SetCursorPosY(cursorPosY + logoPosY);
                ImGui::Image(reinterpret_cast<ImTextureID>(m_AppLogoImGuiTexture->GetImGuiTextureId()),
                             { static_cast<float>(m_AppLogo->GetWidth()) / static_cast<float>(m_AppLogo->GetHeight()) *
                                   logoHeight,
                               logoHeight });

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

                    ImGui::MenuItem("Scene", nullptr, &m_IsDrawScenePanel);
                    ImGui::MenuItem("Viewport", nullptr, &m_IsDrawViewportPanel);
                    ImGui::MenuItem("Assets", nullptr, &m_IsDrawAssetsPanel);
                    ImGui::MenuItem("Props", nullptr, &m_IsDrawPropsPanel);
                    ImGui::MenuItem("Editor Camera", nullptr, &m_IsDrawEditorCameraPanel);

                    ImGui::Separator();

                    if (ImGui::MenuItem("ImGui Demo", nullptr, m_IsDrawImGuiDemoWindow))
                    {
                        m_IsDrawImGuiDemoWindow = !m_IsDrawImGuiDemoWindow;
                    }

                    ImGui::EndMenu();
                }

                ImGui::PopStyleVar();

                DrawGuiPlayStopButton(frameHeight, cursorPosY);

                // Without the custom title bar the system draws the window buttons
                if (isCustomTitleBar && !window->IsTitleBarHasNativeButtons())
                {
                    DrawGuiTitlebarWindowButtons(frameHeight, cursorPosY);
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        if (isCustomTitleBar)
        {
            window->SetTitleBarLayout(std::move(m_TitleBarLayout));
        }

        return frameHeight - frameHeightOld;
    }

    void EditorLayer::DrawGuiTitlebarWindowButtons(float _FrameHeight, float _CursorPosY)
    {
        Ref<Window> window = Application::Get().GetWindow();

        ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
        float buttonWidth = _FrameHeight * 1.2f;
        glm::vec2 buttonSize = { buttonWidth, _FrameHeight };
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth * 3.0f +
                             ImGui::GetStyle().WindowPadding.x);

        if (GuiDrawMenuButton(ICON_FA_MINUS, _CursorPosY, buttonSize))
        {
            window->Minimize();
        }

        if (window->IsWindowMaximized())
        {
            if (GuiDrawMenuButton(ICON_FA_WINDOW_RESTORE, _CursorPosY, buttonSize))
            {
                window->Restore();
            }
        }
        else
        {
            if (GuiDrawMenuButton(ICON_FA_WINDOW_MAXIMIZE, _CursorPosY, buttonSize))
            {
                window->Maximize();
            }
        }

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xff4f4eff);
        if (GuiDrawMenuButton(ICON_FA_XMARK, _CursorPosY, buttonSize))
        {
            Application::Get().Close();
        }
        ImGui::PopStyleColor();

        ImGui::PopStyleColor();

        ImGui::PopStyleVar();
    }

}    // namespace Vega
