#pragma once

#include "Vega/Core/Base.hpp"

#include "glm/glm.hpp"

namespace Vega
{

    class RendererBackend
    {
    public:
        enum class API : uint16_t
        {
            kNone = 0,
            kVulkan,
            kDirectX,
            kOpenGL
        };

    public:
        virtual ~RendererBackend() = default;

        virtual bool Init() = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        void SetClearColor(const glm::vec4& _ClearColor) { m_ClearColor = _ClearColor; }
        const glm::vec4& GetClearColor() const { return m_ClearColor; }

        static API GetAPI() { return s_API; }

        // virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

        static Ref<RendererBackend> Create(API _RendererAPI);

    protected:
        glm::vec4 m_ClearColor { 0.0f, 0.0f, 0.0f, 0.0f };

        static inline API s_API = API::kNone;
    };

}    // namespace Vega
