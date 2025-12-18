#pragma once

#include "Vega/Layers/Layer.hpp"
#include "Vega/Renderer/FrameBuffer.hpp"
#include "Vega/Renderer/Shader.hpp"

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
        Ref<Shader> m_Shader;
        // std::vector<Ref<Texture>> m_ColorBuffers;
        Ref<FrameBuffer> m_FrameBuffer;

        glm::u32vec2 m_ViewportDimensions;
        size_t m_FramesToSkip = 0;

        Ref<Texture> m_AppLogo;
        bool m_IsDrawImGuiDemoWindow = false;
    };

}    // namespace Vega
