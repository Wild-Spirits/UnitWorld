#pragma once

#include <cstdint>

namespace Vega
{

    enum class RenderBufferAllocatorType
    {
        kNone,
        kLinear,
        kFreeList,
    };

    enum class RenderBufferAllocateResultStatus
    {
        kSuccess,
        kOutOfMemory,
    };

    struct RenderBufferAllocateResult
    {
        RenderBufferAllocateResultStatus Status;
        size_t Offset = 0;
    };

    class RenderBufferAllocator
    {
    public:
        RenderBufferAllocator(size_t _TotalSize) : m_TotalSize(_TotalSize) {};
        virtual ~RenderBufferAllocator() = default;

        void SetTotalSize(size_t _Size) { m_TotalSize = _Size; }
        size_t GetTotalSize() const { return m_TotalSize; }
        virtual RenderBufferAllocateResult Allocate(size_t _Size) = 0;
        virtual void Clear() = 0;

    protected:
        size_t m_TotalSize = 0;
    };

    class RenderBufferLinearAllocator : public RenderBufferAllocator
    {
    public:
        RenderBufferLinearAllocator(size_t _TotalSize);
        virtual ~RenderBufferLinearAllocator() override = default;
        RenderBufferAllocateResult Allocate(size_t _Size) override;
        void Clear() override { m_CurrentOffset = 0; }

    protected:
        size_t m_CurrentOffset = 0;
    };

}    // namespace Vega
