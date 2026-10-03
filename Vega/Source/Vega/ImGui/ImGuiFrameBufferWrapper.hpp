#pragma once

namespace Vega
{

    class ImGuiFrameBufferWrapper
    {
    public:
        ImGuiFrameBufferWrapper() = default;

        virtual ~ImGuiFrameBufferWrapper() = default;

        virtual void OnDetach() = 0;

        virtual void* GetImGuiColorAttachmentId(size_t _AttachmentIndex = 0) const = 0;
        virtual void* GetImGuiColorDepthAttachmentId(size_t _AttachmentIndex = 0) const = 0;
    };

}    // namespace Vega
