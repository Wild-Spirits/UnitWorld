#include "RenderBufferAllocator.hpp"

namespace Vega
{

    RenderBufferLinearAllocator::RenderBufferLinearAllocator(size_t _TotalSize)
        : RenderBufferAllocator(_TotalSize),
          m_CurrentOffset(0)
    { }

    RenderBufferAllocateResult RenderBufferLinearAllocator::Allocate(size_t _Size)
    {
        if (m_CurrentOffset + _Size > m_TotalSize)
        {
            return RenderBufferAllocateResult { RenderBufferAllocateResultStatus::kOutOfMemory, m_CurrentOffset };
        }

        size_t allocatedOffset = m_CurrentOffset;
        m_CurrentOffset += _Size;
        return RenderBufferAllocateResult { RenderBufferAllocateResultStatus::kSuccess, allocatedOffset };
    }

}    // namespace Vega
