#pragma once

#include "Vega/Core/Base.hpp"
#include "Vega/Core/Timestep.hpp"
#include "Vega/Events/EventManager.hpp"

namespace Vega
{

    class Layer
    {
    public:
        Layer(std::string_view _Name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach(Ref<EventManager> _EventManager) { }
        virtual void OnDetach() { }

        virtual void OnUpdate(Timestep _Timestep) { }
        virtual void OnRender() { }
        virtual void OnGuiRender() { }

        std::string_view GetName() const { return m_Name; }

    protected:
        std::string m_Name;
    };

}    // namespace Vega
