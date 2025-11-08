#pragma once

#include "FrameBuffer.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Vega/Core/Base.hpp"

#include "Vega/Core/Window.hpp"
#include "Vega/ImGui/ImGuiImpl.hpp"
#include "Vega/Plugins/PluginLibrary.hpp"
#include "Vega/Renderer/RendererBackendApi.hpp"

namespace Vega
{

    template <class T>
    std::vector<std::vector<Ref<T>>> CastAttachmentsTo(std::vector<std::vector<Ref<Texture>>> _Targets)
    {
        std::vector<std::vector<Ref<T>>> colorTargetsT;
        colorTargetsT.resize(_Targets.size());

        for (size_t i = 0; i < _Targets.size(); ++i)
        {
            colorTargetsT[i].reserve(_Targets[i].size());
            for (auto& tex : _Targets[i])
            {
                colorTargetsT[i].push_back(std::static_pointer_cast<T>(tex));
            }
        }

        return colorTargetsT;
    }

    class RendererBackend
    {
    public:
        struct CreateReturnValue
        {
            Ref<PluginLibrary> Plugin;
            Ref<RendererBackend> Backend;
        };

    public:
        virtual ~RendererBackend() = default;

        virtual bool Init() = 0;
        virtual void Shutdown() = 0;

        virtual bool OnWindowCreate(Ref<Window> _Window) = 0;
        virtual void OnWindowDestroy(Ref<Window> _Window) = 0;
        virtual bool OnResize() = 0;

        virtual bool FramePrepareWindowSurface() = 0;
        virtual void FrameCommandListBegin() = 0;

        // TODO: add color and depth/stencil attachments in other way ?
        virtual void BeginRendering(glm::ivec2 _ViewportOffset, glm::uvec2 _ViewportSize,
                                    std::vector<std::vector<Ref<Texture>>> _ColorTargets,
                                    std::vector<std::vector<Ref<Texture>>> _DepthStencilTargets) = 0;
        virtual void TestFoo() {};

        virtual void EndRendering() = 0;
        virtual void FrameCommandListEnd() = 0;
        virtual void FrameSubmit() = 0;
        virtual void TmpRendergraphExecute() = 0;
        virtual void FramePresent() = 0;

        void SetClearColor(const glm::vec4& _ClearColor) { m_ClearColor = _ClearColor; }
        const glm::vec4& GetClearColor() const { return m_ClearColor; }

        virtual std::vector<Ref<Texture>> GetSwapchainColorTextures() = 0;
        virtual void SetActiveViewport(glm::vec2 _Start, glm::vec2 _Size) = 0;

        static RendererBackendApi GetAPI() { return s_API; }

        // virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

        static CreateReturnValue Create(RendererBackendApi _RendererAPI);

        virtual Ref<Shader> CreateShader(const ShaderConfig& _ShaderConfig,
                                         const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) = 0;
        virtual Ref<Texture> CreateTexture(std::string_view _Name, const TextureProps& _Props) = 0;
        // virtual Ref<Texture> CreateTexture(std::string_view _Name, TextureProps _Props, uint8_t _Data) = 0;

        virtual Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferProps& _Props) = 0;

        virtual Ref<ImGuiImpl> CreateImGuiImpl() = 0;

    protected:
        static CreateReturnValue CreateVulkanRendererBackend();
        static CreateReturnValue CreateOpenGlRendererBackend();

    protected:
        glm::vec4 m_ClearColor { 0.0f, 0.0f, 0.0f, 0.0f };

        static inline RendererBackendApi s_API = RendererBackendApi::kNone;
    };

}    // namespace Vega
