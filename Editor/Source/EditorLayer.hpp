#pragma once

#include "EditorCamera.hpp"
#include "Panels/EntityPropsPanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Vega/Core/Window.hpp"
#include "Vega/ImGui/ImGuiFrameBufferWrapper.hpp"
#include "Vega/ImGui/ImGuiTextureWrapper.hpp"
#include "Vega/Layers/Layer.hpp"
#include "Vega/Renderer/FrameBuffer.hpp"
#include "Vega/Renderer/Sampler.hpp"
#include "Vega/Scene/Scene.hpp"

#include <glm/glm.hpp>

namespace Vega
{

    enum class SceneState
    {
        // Editor camera, scene logic is not running
        kEdit,
        // Primary scene camera (TODO: copy the scene and run its logic)
        kPlay,
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

        void DrawGuiPlayStopButton(float _FrameHeight, float _CursorPosY);
        void DrawGuiViewport();

        void ResizeViewportFrameBuffer();
        RenderView GetActiveRenderView();

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
        SceneState m_SceneState = SceneState::kEdit;
        EditorCamera m_EditorCamera;
        // Camera input is read in OnGuiRender, where the viewport hover state is known
        Timestep m_LastTimestep;
        SceneHierarchyPanel m_SceneHierarchyPanel;
        EntityPropsPanel m_EntityPropsPanel;
    };

}    // namespace Vega
