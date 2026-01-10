#include "RenderBuffer.hpp"
#include "RenderBufferAllocator.hpp"
#include "Vega/Core/Assert.hpp"

namespace Vega
{

    RenderBuffer::RenderBuffer(const RenderBufferProps& _Props) : m_RenderBufferProps(_Props)
    {
        switch (_Props.AllocatorType)
        {
            case RenderBufferAllocatorType::kLinear:
                m_Allocator = CreateScope<RenderBufferLinearAllocator>(_Props.ElementSize * _Props.ElementCount);
                break;
            case RenderBufferAllocatorType::kFreeList:
                VEGA_CORE_ASSERT("RenderBufferAllocatorType::kFreeList not implemented yet!");
            case RenderBufferAllocatorType::kNone:
            default: break;
        }
    }

    void RenderBuffer::Destroy()
    {
        DestroyInternal();
        m_Allocator.reset();
    }

    void RenderBuffer::Clear(bool _IsNeedZeroMemory)
    {
        if (_IsNeedZeroMemory)
        {
            // Implement zeroing memory logic here
        }

        if (m_Allocator)
        {
            m_Allocator->Clear();
            // Implement clearing logic using allocator here
        }
    }

    size_t RenderBuffer::LoadRange(size_t _Size, const void* _Data, bool _IncludeInFrameWorkload)
    {
        // TODO: Handle owerflow without allocator (if m_Allocator is nullptr)
        RenderBufferAllocateResult allocateResult = {
            .Status = RenderBufferAllocateResultStatus::kSuccess,
            .Offset = 0,
        };
        if (m_Allocator)
        {
            allocateResult = m_Allocator->Allocate(_Size);
            VEGA_CORE_ASSERT(allocateResult.Status == RenderBufferAllocateResultStatus::kSuccess,
                             "RenderBuffer LoadRange: Out of memory! Resize is not supported yet.");
        }
        LoadRangeInternal(allocateResult.Offset, _Size, _Data, _IncludeInFrameWorkload);
        return allocateResult.Offset;
    }

}    // namespace Vega
