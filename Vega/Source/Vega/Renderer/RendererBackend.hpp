#include "Vega/Core/Base.hpp"

#include "glm/glm.hpp"

namespace LM
{

    enum class RendererAPI : uint16_t
    {
        kNone = 0,
        kVulkan,
        kDirectX,
        kOpenGl
    };

    class RendererBackend
    {
    public:
        virtual ~RendererBackend() = default;

        virtual void Init() = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        void SetClearColor(const glm::vec4& _ClearColor) { m_ClearColor = _ClearColor; }
        const glm::vec4& GetClearColor() const { return m_ClearColor; }

        //virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

        static Ref<RendererBackend> Create(RendererAPI _RendererBackendType);

    protected:
        glm::vec4 m_ClearColor { 0.0f, 0.0f, 0.0f, 0.0f };

        static inline RendererAPI s_RendererBackendType = RendererAPI::kNone;
    };

}    // namespace LM
