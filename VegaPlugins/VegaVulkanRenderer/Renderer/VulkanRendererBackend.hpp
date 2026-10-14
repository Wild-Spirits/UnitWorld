#pragma once

#include "Vega/Renderer/RendererBackend.hpp"
#include "VulkanBase.hpp"
#include "VulkanDeviceWrapper.hpp"
#include "VulkanRenderBuffer.hpp"
#include "VulkanSwapchain.hpp"

#include <deque>
#include <functional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    class VulkanRendererBackend : public RendererBackend
    {
    public:
        VulkanRendererBackend();
        virtual ~VulkanRendererBackend() = default;

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual void WaitIdle() override;

        // TODO: Add per window resources to be able to create multiple windows (not imgui)
        virtual bool OnWindowCreate(Ref<Window> _Window) override;
        virtual void OnWindowDestroy(Ref<Window> _Window) override;

        virtual bool OnResize() override;

        bool FramePrepareWindowSurface() override;
        void FrameCommandListBegin() override;
        void FrameCommandListEnd() override;
        void FrameSubmit() override;
        void TmpRendergraphExecute() override;
        void FramePresent() override;

        void ClearColorTexture();
        void ColorTexturePepareForPresent();

        /**
         * @brief Retrieves the Vulkan context.
         *
         * This function returns a constant reference to the Vulkan context (VkContext)
         * associated with the renderer backend.
         *
         * @return const VkContext& A constant reference to the Vulkan context.
         */
        inline const VulkanContext& GetVkContext() const { return m_VkContext; }

        /**
         * @brief Retrieves the Vulkan device wrapper.
         *
         * This function returns a constant reference to the VulkanDeviceWrapper instance
         * associated with the renderer backend. The VulkanDeviceWrapper encapsulates the
         * Vulkan device and related resources.
         *
         * @return const VulkanDeviceWrapper& A constant reference to the VulkanDeviceWrapper instance.
         */
        inline const VulkanDeviceWrapper& GetVkDeviceWrapper() const { return m_VkDeviceWrapper; }

        /**
         * @brief Retrieves the Vulkan swapchain.
         *
         * This function returns a constant reference to the VulkanSwapchain instance
         * associated with the renderer backend. The VulkanSwapchain encapsulates the
         * Vulkan swapchain and related resources.
         *
         * @return const VulkanSwapchain& A constant reference to the VulkanSwapchain instance.
         */
        inline const VulkanSwapchain& GetVkSwapchain() const { return m_VkSwapchain; }

        VkCommandBuffer GetCurrentGraphicsCommandBuffer() const;
        Ref<VulkanRenderBuffer> GetCurrentStagingBuffer() const;

        uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }
        uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }

        // TODO: add color and depth/stencil attachments in other way ?
        void BeginRendering(const glm::ivec2& _ViewportOffset, const glm::uvec2& _ViewportSize,
                            Ref<FrameBuffer> _FrameBuffer) override;

        virtual void DrawIndexed(uint32_t _IndexCount = 0) override;

        void EndRendering() override;

        void VulkanBeginRendering(const glm::ivec2& _ViewportOffset, const glm::uvec2& _ViewportSize,
                                  const std::vector<std::vector<Ref<VulkanTexture>>>& _ColorTargets = {},
                                  const std::vector<std::vector<Ref<VulkanTexture>>>& _DepthStencilTargets = {});
        void VulkanEndRendering();

        std::vector<Ref<Texture>> GetSwapchainColorTextures() override { return m_VkSwapchain.GetColorTextures(); }
        void SetActiveViewport(glm::vec2 _Start, glm::vec2 _Size) override;

        void CreateImGuiDescriptorPool();
        inline VkDescriptorPool GetImGuiDescriptorPool() { return m_ImGuiDescriptorPool; }
        void DestroyImGuiDescriptorPool();

        Ref<ImGuiImpl> CreateImGuiImpl() override;

        Ref<ImGuiTextureWrapper> CreateImGuiTextureWrapper(Ref<Texture> _Texture, Ref<Sampler> _Sampler) override;
        Ref<ImGuiFrameBufferWrapper> CreateImGuiFrameBufferWrapper(Ref<FrameBuffer> _FrameBuffer,
                                                                   Ref<Sampler> _Sampler) override;

        Ref<Shader> CreateShader(const ShaderConfig& _ShaderConfig,
                                 const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) override;

        VkCommandBuffer CreateAndBeginSingleUseCommandBuffer();
        void DestroyAndEndSingleUseCommandBuffer(VkCommandBuffer _CommandBuffer, VkQueue _Queue,
                                                 VkCommandPool _CommandPool);
        // NOTE: Doesn't wait for the GPU: the command buffer is freed via the deletion queue
        void EndAndSubmitSingleUseCommandBuffer(VkCommandBuffer _CommandBuffer);

        /**
         * @brief Defers destruction of GPU objects until the GPU is done with them.
         *
         * The deleter runs once all frames submitted up to the current one are complete, so objects used by frames
         * in flight can be "destroyed" without waiting for the device to become idle.
         * The deleter must capture Vulkan handles by value, not the objects owning them.
         */
        void EnqueueDeletion(std::function<void()> _Deleter);

        Ref<Texture> CreateTexture(std::string_view _Name, const TextureProps& _Props) override;
        Ref<Texture> CreateTexture(std::string_view _Name, const TextureProps& _Props, uint8_t* _Data) override;

        Ref<Sampler> CreateSampler(std::string_view _Name, const SamplerProps& _Props) override;

        Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferProps& _Props) override;

        Ref<RenderBuffer> CreateRenderBuffer(const RenderBufferProps& _Props) override;

        /**
         * @brief Retrieves the singleton instance of the VulkanRendererBackend.
         *
         * @return VulkanRendererBackend* Pointer to the singleton instance of the VulkanRendererBackend.
         */
        static inline VulkanRendererBackend* GetVkRendererBackend() { return m_Instance; }

    private:
        bool RecreateSwapchain();
        bool CreateGraphicsCommandBuffer(Ref<Window> _Window);

        // TODO: Add arg to select winding mode
        void SetWinding();

        void SetStencilReference(uint32_t _StencilReference);
        void SetStencilCompareMask(uint32_t _StencilCompareMask);
        void SetStencilOp(VkStencilOp _FailOp, VkStencilOp _PassOp, VkStencilOp _DepthFailOp, VkCompareOp _CompareOp);
        void SetStencilWriteMask(uint32_t _StencilWriteMask);
        void SetStencilTestEnabled(bool _StencilTestEnabled);

        void SetDepthTestEnabled(bool _DepthTestEnabled);
        void SetDepthWriteEnabled(bool _DepthWriteEnabled);

        Ref<VulkanTexture> GetCurrentColorTexture() const;

        // TODO: Move CommandBuffer funcs to separate class
        void CommandBufferReset(VkCommandBuffer _VkCommandBuffer);
        void CommandBufferBegin(VkCommandBuffer _VkCommandBuffer, bool _IsSingleUse, bool _IsRenderpassContinue,
                                bool _IsSimultaneousUse);
        void CommandBufferEnd(VkCommandBuffer _VkCommandBuffer);
        void CommandBufferUpdateSubmited(VkCommandBuffer _VkCommandBuffer);

        // Runs deleters of all frames up to _CompletedFrameNumber inclusive
        void FlushDeletionQueue(uint64_t _CompletedFrameNumber);

    private:
        static void VerifyRequiredExtensions(const std::vector<const char*>& _RequiredExtensions);

        static void VerifyValidationLayers(const std::vector<const char*>& _RequiredValidationLayers);

    protected:
        // @brief Hi
        VkSurfaceKHR m_VkSurface;

        VulkanContext m_VkContext = {};

        RendererBackendConfig::Flags m_RendererFlags = RendererBackendConfig::kRendererConfigFlagVsyncEnabledBit |
                                                       RendererBackendConfig::kRendererConfigFlagEnableValidation;

        VulkanDeviceWrapper m_VkDeviceWrapper;

        VulkanSwapchain m_VkSwapchain;
        std::vector<VkCommandBuffer> m_GraphicsCommandBuffer;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_QueueCompleteSemaphores;
        std::vector<VkFence> m_InFlightFences;

        std::vector<Ref<VulkanRenderBuffer>> m_StagingBuffers;

        std::vector<Ref<VulkanTexture>> m_DepthBufferTextures;

        bool m_IsNeedRecreateSwapchain = false;

        uint32_t m_CurrentFrame = 0;
        uint32_t m_ImageIndex = 0;

        struct DeferredDeletion
        {
            uint64_t FrameNumber;
            std::function<void()> Deleter;
        };

        // NOTE: Frame numbers grow monotonically (unlike m_CurrentFrame) and start from 1, 0 means "nothing submitted"
        uint64_t m_FrameNumber = 1;
        // Frame number submitted with the in-flight fence of each frame index
        std::vector<uint64_t> m_InFlightFrameNumbers;
        // Sorted by FrameNumber, since entries are only appended with the current frame number
        std::deque<DeferredDeletion> m_DeletionQueue;

        VkDescriptorPool m_ImGuiDescriptorPool = nullptr;

        static inline VulkanRendererBackend* m_Instance = nullptr;
    };

}    // namespace Vega
