#pragma once

#include "Panels/EntityPropsPanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Vega/Layers/Layer.hpp"
#include "Vega/Renderer/FrameBuffer.hpp"
#include "Vega/Scene/Scene.hpp"

namespace Vega
{

    class EditorLayer : public Layer
    {
    public:
        void OnAttach(Ref<EventManager> _EventManager) override;

        void OnDetach() override;

        void OnUpdate() override;

        void OnRender() override;

        void OnGuiRender() override;

    protected:
        bool GuiDrawBeginMenu(std::string_view _Title);

        bool GuiDrawMenuButton(std::string_view _Title, float _CursorPosY, const glm::vec2& _Size);

        float DrawGuiTitlebar();

    protected:
        // std::vector<Ref<Texture>> m_ColorBuffers;
        Ref<FrameBuffer> m_FrameBuffer;

        glm::u32vec2 m_ViewportDimensions;

        Ref<Texture> m_AppLogo;
        bool m_IsDrawImGuiDemoWindow = false;

        Ref<Scene> m_ActiveScene;
        SceneHierarchyPanel m_SceneHierarchyPanel;
        EntityPropsPanel m_EntityPropsPanel;
    };

}    // namespace Vega
