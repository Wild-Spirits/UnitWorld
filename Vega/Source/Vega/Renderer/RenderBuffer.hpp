#pragma once

#include "RenderBufferAllocator.hpp"
#include "Vega/Core/Base.hpp"

#include <cstddef>
#include <string>

namespace Vega
{

    enum class RenderBufferType
    {
        kVertex,
        kIndex,
        kUniform,
        kStaging,
        kRead,
        kStorage,
    };

    struct RenderBufferProps
    {
        std::string Name;
        RenderBufferType Type;
        size_t ElementSize;
        size_t ElementCount;
        RenderBufferAllocatorType AllocatorType = RenderBufferAllocatorType::kNone;
    };

    class RenderBuffer
    {
    public:
        RenderBuffer(const RenderBufferProps& _Props);
        virtual ~RenderBuffer() = default;

        void Destroy();
        void Clear(bool _IsNeedZeroMemory = false);
        size_t LoadRange(size_t _Size, const void* _Data, bool _IncludeInFrameWorkload);

        virtual void Bind(size_t _Offset) = 0;

    protected:
        virtual void DestroyInternal() = 0;
        virtual void LoadRangeInternal(size_t _Offset, size_t _Size, const void* _Data,
                                       bool _IncludeInFrameWorkload) = 0;

    protected:
        RenderBufferProps m_RenderBufferProps;

        Scope<RenderBufferAllocator> m_Allocator = nullptr;
    };

}    // namespace Vega
