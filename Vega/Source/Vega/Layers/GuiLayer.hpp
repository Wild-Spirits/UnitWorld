#pragma once

#include "Layer.hpp"

namespace Vega
{

    class GuiLayer : public Layer
    {
    public:
        GuiLayer(std::string_view name = "GuiLayer") : Layer(name) { }
        virtual ~GuiLayer() = default;

        virtual void BeginGuiFrame() = 0;
        virtual void EndGuiFrame() = 0;

    protected:
    };

}    // namespace Vega
