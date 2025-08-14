#pragma once

#include "Vega/Layers/Layer.hpp"
#include "Vega/Renderer/Shader.hpp"

namespace Vega
{

    class EditorLayer : public Layer
    {
    public:
        void OnAttach(Ref<EventManager> _EventManager) override;

        void OnUpdate() override;

        void OnRender() override;

        void OnGuiRender() override;

    protected:
        Ref<Shader> m_Shader;
    };

}    // namespace Vega
