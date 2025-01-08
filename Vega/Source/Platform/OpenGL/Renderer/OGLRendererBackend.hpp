#pragma once

#include "Vega/Renderer/RendererBackend.hpp"

namespace Vega
{

    class OGLRendererBackend : public RendererBackend
    {
    public:
        virtual ~OGLRendererBackend() = default;

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual bool OnWindowCreate(/* Ref<Wiondow> _Window */) override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;
    };

}    // namespace Vega
