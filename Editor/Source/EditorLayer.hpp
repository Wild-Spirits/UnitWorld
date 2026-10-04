#pragma once

#include "Panels/EntityPropsPanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Vega/Core/Window.hpp"
#include "Vega/ImGui/ImGuiFrameBufferWrapper.hpp"
#include "Vega/ImGui/ImGuiTextureWrapper.hpp"
#include "Vega/Layers/Layer.hpp"
#include "Vega/Renderer/FrameBuffer.hpp"
#include "Vega/Renderer/Sampler.hpp"
#include "Vega/Scene/Scene.hpp"
#include "Vega/Scene/Systems/SceneSystemStaticMeshDraw.hpp"

#include <glm/glm.hpp>

namespace Vega
{

    // TODO: Tmp until CameraComponent / editor camera
    struct EditorTestCamera
    {
        glm::vec3 Position { 0.0f, 0.0f, 3.0f };
        // Euler angles in degrees (pitch, yaw, roll), camera looks along -Z with zero rotation
        glm::vec3 RotationDeg { 0.0f, 0.0f, 0.0f };
        float FovYDeg = 45.0f;
        float Near = 0.1f;
        float Far = 100.0f;

        glm::mat4 View { 1.0f };
        glm::mat4 Projection { 1.0f };
        // When set, View and Projection are edited directly and not recalculated from the params above
        bool IsManualMatrices = false;
    };

    class EditorLayer : public Layer
    {
    public:
        void OnAttach(Ref<EventManager> _EventManager) override;

        void OnDetach() override;

        void OnUpdate(Timestep _Timestep) override;

        void OnRender() override;

        void OnGuiRender() override;

    protected:
        bool GuiDrawBeginMenu(std::string_view _Title);

        bool GuiDrawMenuButton(std::string_view _Title, float _CursorPosY, const glm::vec2& _Size);

        // Adds the last ImGui item to the title bar items that don't drag the window
        void AddTitleBarInteractiveItem(const glm::vec2& _ExtraPadding = { 0.0f, 0.0f });

        float DrawGuiTitlebar();
        void DrawGuiTitlebarWindowButtons(float _FrameHeight, float _CursorPosY);

        void UpdateTestCameraMatrices();
        void DrawGuiTestCamera();

    protected:
        // std::vector<Ref<Texture>> m_ColorBuffers;
        Ref<FrameBuffer> m_FrameBuffer;
        Ref<Sampler> m_FrameBufferSampler;
        Ref<ImGuiFrameBufferWrapper> m_FrameBufferImGuiTexture;

        glm::u32vec2 m_ViewportDimensions;

        Ref<Texture> m_AppLogo;
        Ref<Sampler> m_AppLogoSampler;
        Ref<ImGuiTextureWrapper> m_AppLogoImGuiTexture;
        bool m_IsDrawImGuiDemoWindow = false;
        WindowTitleBarLayout m_TitleBarLayout;

        Ref<Scene> m_ActiveScene;
        Ref<SceneSystems::SceneSystemStaticMeshDraw> m_StaticMeshDrawSystem;
        EditorTestCamera m_TestCamera;
        SceneHierarchyPanel m_SceneHierarchyPanel;
        EntityPropsPanel m_EntityPropsPanel;
    };

}    // namespace Vega
