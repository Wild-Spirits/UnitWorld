#pragma once

#include "Vega/Renderer/FrameBuffer.hpp"
#include "Vega/Renderer/RendererBackend.hpp"

namespace Vega
{

    // TODO: Implement methods with placeholders
    class OpenGlRendererBackend : public RendererBackend
    {
    public:
        virtual ~OpenGlRendererBackend() = default;

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual bool OnWindowCreate(Ref<Window> _Window) override;
        virtual void OnWindowDestroy(Ref<Window> _Window) override;

        virtual bool OnResize() override;

        bool FramePrepareWindowSurface() override;
        void FrameCommandListBegin() override;
        void BeginRendering(const glm::ivec2& _ViewportOffset, const glm::uvec2& _ViewportSize,
                            Ref<FrameBuffer> _FrameBuffer) override
        { }
        void EndRendering() override { }
        void FrameCommandListEnd() override;
        void FrameSubmit() override;
        void TmpRendergraphExecute() override;
        void FramePresent() override;

        std::vector<Ref<Texture>> GetSwapchainColorTextures() override { return {}; }
        void SetActiveViewport(glm::vec2 _Start, glm::vec2 _Size) override { }

        Ref<ImGuiImpl> CreateImGuiImpl() override;

        // TODO: implement
        Ref<Shader> CreateShader(const ShaderConfig& _ShaderConfig,
                                 const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) override
        {
            return nullptr;
        }

        // TODO: implement
        Ref<Texture> CreateTexture(std::string_view _Name, const TextureProps& _Props) override { return nullptr; }
        Ref<Texture> CreateTexture(std::string_view _Name, TextureProps _Props, uint8_t* _Data) override
        {
            return nullptr;
        }

        // TODO: implement
        Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferProps& _Props) override { return nullptr; }
    };

}    // namespace Vega
