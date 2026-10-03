#pragma once

#include "Vega/Core/Base.hpp"
#include "Vega/Renderer/FrameBuffer.hpp"
#include "Vega/Renderer/RenderGraph/RenderGraphNode.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace Vega
{

    class RenderGraph
    {
    public:
    protected:
        std::vector<std::unordered_map<std::string, Ref<RenderGraphNode>>> m_ParallelNodeLayers;
        Ref<FrameBuffer> m_MainSink;
    };

}    // namespace Vega
