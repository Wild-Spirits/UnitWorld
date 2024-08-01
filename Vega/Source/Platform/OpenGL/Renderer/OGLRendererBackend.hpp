#pragma once

#include "Vega/Renderer/RendererBackend.hpp"

namespace LM
{

    class OGLRendererBackend : public RendererBackend
    {
    public:
        virtual ~OGLRendererBackend() = default;

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;
    };

}    // namespace LM
