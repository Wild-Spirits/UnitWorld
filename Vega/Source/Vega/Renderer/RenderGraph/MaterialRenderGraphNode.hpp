#pragma once

#include "Vega/Managers/StaticMeshManager.hpp"
#include "Vega/Renderer/Material.hpp"

#include <unordered_map>
#include <vector>

namespace Vega
{

    struct MaterialRenderGraphNodeItemPerMaterial
    {
        StaticMeshManagerMeshInfo MeshInfo;
    };

    class MaterialRenderGraphNode
    {
    public:
    protected:
        std::unordered_map<Ref<Material>, std::vector<MaterialRenderGraphNodeItemPerMaterial>> m_MaterialProperties;
    };

}    // namespace Vega
