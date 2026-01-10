#pragma once

#include "Manager.hpp"
#include "Vega/Renderer/RenderBuffer.hpp"

#include "glm/ext/vector_float3.hpp"

#include <string>
#include <unordered_map>

namespace Vega
{

    struct StaticMeshVertex
    {
        glm::vec3 Position;
    };

    typedef uint32_t StaticMeshIndex;

    struct StaticMeshManagerMeshInfo
    {
        size_t VertexOffset;
        size_t VertexCount;
        size_t IndexOffset;
        size_t IndexCount;

        // TODO: May need add reference count for mesh usage tracking and auto release if set to autorelease

        // TODO: Add BoundingBox, BoundingSphere, etc. (may be in separate struct and manager)
    };

    class StaticMeshManager : public Manager
    {
    public:
        StaticMeshManager();
        virtual ~StaticMeshManager() = default;

        virtual void Destroy() override;

        StaticMeshManagerMeshInfo AddMesh(std::string_view _MeshName, const StaticMeshVertex* _Vertices,
                                          size_t _VertexCount, const StaticMeshIndex* _Indices, size_t _IndexCount,
                                          bool _IncludeInFrameWorkload);

        void BindMesh(std::string_view _MeshName);

    protected:
        // TODO: friend class AssetManager;
    protected:
        Ref<RenderBuffer> m_VertexBuffer;
        Ref<RenderBuffer> m_IndexBuffer;
        std::unordered_map<std::string, StaticMeshManagerMeshInfo> m_MeshesInfo;
    };

}    // namespace Vega
