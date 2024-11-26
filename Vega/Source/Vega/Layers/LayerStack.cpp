#include "LayerStack.hpp"

namespace Vega
{

    LayerStack::~LayerStack()
    {
        for (Ref<Layer> layer : m_Layers)
        {
            layer->OnDetach();
        }
    }

    void LayerStack::PushLayer(Ref<Layer> _Layer)
    {
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, _Layer);
        m_LayerInsertIndex++;
    }

    void LayerStack::PushOverlay(Ref<Layer> _Overlay) { m_Layers.emplace_back(_Overlay); }

    void LayerStack::PopLayer(Ref<Layer> _Layer)
    {
        auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, _Layer);
        if (it != m_Layers.begin() + m_LayerInsertIndex)
        {
            _Layer->OnDetach();
            m_Layers.erase(it);
            m_LayerInsertIndex--;
        }
    }

    void LayerStack::PopOverlay(Ref<Layer> _Overlay)
    {
        auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), _Overlay);
        if (it != m_Layers.end())
        {
            _Overlay->OnDetach();
            m_Layers.erase(it);
        }
    }

}    // namespace Vega
