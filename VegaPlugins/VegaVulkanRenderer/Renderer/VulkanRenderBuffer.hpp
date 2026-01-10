#pragma once

#include "Vega/Renderer/RenderBuffer.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    struct VulkanRenderBufferInfoByType
    {
        VkBufferUsageFlags Usage;
        VkMemoryPropertyFlags MemoryProperties;
    };

    class VulkanRenderBuffer : public RenderBuffer
    {
    public:
    public:
        VulkanRenderBuffer(const RenderBufferProps& _Props);

        virtual ~VulkanRenderBuffer() override = default;

        // void Resize(size_t _NewElementCount);

        virtual void Bind(size_t _Offset = 0) override;
        // void Unbind();

        // void MapMemory();
        // void UnmapMemory();

        // void Flush();

        // void Read();

        VkBuffer GetVkBuffer() const { return m_VkBuffer; }

    protected:
        virtual void DestroyInternal() override;

        void LoadRangeInternal(size_t _Offset, size_t _Size, const void* _Data, bool _IncludeInFrameWorkload) override;

        void CopyRangeInternal(size_t _SrcOffset, VkBuffer _SrcBuffer, size_t _DstOffset, size_t _Size,
                               bool _IncludeInFrameWorkload);

        VulkanRenderBufferInfoByType GetVulkanRenderBufferInfoByType(RenderBufferType _Type);

        bool IsVulkanRenderBufferHostVisible();
        bool IsVulkanRenderBufferDeviceLocal();
        bool IsVulkanRenderBufferHostCoherent();

    protected:
        VkBuffer m_VkBuffer = nullptr;

        VulkanRenderBufferInfoByType m_VkRenderBufferInfo;

        VkMemoryRequirements m_MemoryRequirements;
        VkDeviceMemory m_BufferMemory;
    };

}    // namespace Vega
