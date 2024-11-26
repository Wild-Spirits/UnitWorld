#pragma once

#include "Vega/Core/Base.hpp"
#include "Vega/Events/EventManager.hpp"

namespace Vega
{

    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach(Ref<EventManager> _EventManager) { }
        virtual void OnDetach() { }

        virtual void OnUpdate() { }
        virtual void OnRender() { }

        const std::string& GetName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };

}    // namespace Vega
