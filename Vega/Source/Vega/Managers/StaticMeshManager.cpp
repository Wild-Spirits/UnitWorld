#include "StaticMeshManager.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Assert.hpp"
#include "Vega/Renderer/RendererBackend.hpp"

namespace Vega
{

    StaticMeshManager::StaticMeshManager()
    {
        Ref<RendererBackend> rendererBackend = Application::Get().GetRendererBackend();

        m_VertexBuffer = rendererBackend->CreateRenderBuffer(RenderBufferProps {
            .Name = "StaticMeshManager_VertexBuffer",
            .Type = RenderBufferType::kVertex,
            .ElementSize = sizeof(StaticMeshVertex),
            .ElementCount = 1024 * 1024,    // TODO: Make configurable
            .AllocatorType = RenderBufferAllocatorType::kLinear,
            // TODO: .AllocatorType = RenderBufferAllocatorType::kFreeList,
        });

        m_IndexBuffer = rendererBackend->CreateRenderBuffer(RenderBufferProps {
            .Name = "StaticMeshManager_IndexBuffer",
            .Type = RenderBufferType::kIndex,
            .ElementSize = sizeof(StaticMeshIndex),
            .ElementCount = 1024 * 1024 * 4,    // TODO: Make configurable
            .AllocatorType = RenderBufferAllocatorType::kLinear,
            // TODO: .AllocatorType = RenderBufferAllocatorType::kFreeList,
        });
    }

    void StaticMeshManager::Destroy()
    {
        m_VertexBuffer->Destroy();
        m_IndexBuffer->Destroy();
    }

    StaticMeshManagerMeshInfo StaticMeshManager::AddMesh(std::string_view _MeshName, const StaticMeshVertex* _Vertices,
                                                         size_t _VertexCount, const StaticMeshIndex* _Indices,
                                                         size_t _IndexCount, bool _IncludeInFrameWorkload)
    {
        StaticMeshManagerMeshInfo meshInfo;
        meshInfo.VertexOffset =
            m_VertexBuffer->LoadRange(_VertexCount * sizeof(StaticMeshVertex), _Vertices, _IncludeInFrameWorkload);
        meshInfo.VertexCount = _VertexCount;
        meshInfo.IndexOffset =
            m_IndexBuffer->LoadRange(_IndexCount * sizeof(StaticMeshIndex), _Indices, _IncludeInFrameWorkload);
        meshInfo.IndexCount = _IndexCount;

        m_MeshesInfo[_MeshName.data()] = meshInfo;

        return meshInfo;
    }

    void StaticMeshManager::BindMesh(std::string_view _MeshName)
    {
        VEGA_CORE_ASSERT(!_MeshName.empty(), "StaticMeshManager::BindMesh: Mesh name is empty!");
        auto meshInfoIt = m_MeshesInfo.find(_MeshName.data());
        VEGA_CORE_ASSERT(meshInfoIt != m_MeshesInfo.end(), "StaticMeshManager::BindMesh: Mesh not found!");

        m_VertexBuffer->Bind(meshInfoIt->second.VertexOffset);
        m_IndexBuffer->Bind(meshInfoIt->second.IndexOffset);
    }

}    // namespace Vega
