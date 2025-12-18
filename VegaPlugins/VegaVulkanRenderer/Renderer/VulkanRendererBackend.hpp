#pragma once

#include "Vega/Renderer/RendererBackend.hpp"
#include "VulkanBase.hpp"
#include "VulkanDeviceWrapper.hpp"
#include "VulkanSwapchain.hpp"

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

        uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }
        uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }

        // TODO: add color and depth/stencil attachments in other way ?
        void BeginRendering(const glm::ivec2& _ViewportOffset, const glm::uvec2& _ViewportSize,
                            Ref<FrameBuffer> _FrameBuffer) override;

        void TestFoo() override;

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

        Ref<Shader> CreateShader(const ShaderConfig& _ShaderConfig,
                                 const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) override;

        VkCommandBuffer CreateAndBeginSingleUseCommandBuffer();
        void DestroySingleUseCommandBuffer(VkCommandBuffer _CommandBuffer);

        Ref<Texture> CreateTexture(std::string_view _Name, const TextureProps& _Props) override;
        Ref<Texture> CreateTexture(std::string_view _Name, TextureProps _Props, uint8_t* _Data) override;

        Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferProps& _Props) override;

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

        std::vector<Ref<VulkanTexture>> m_DepthBufferTextures;

        bool m_IsNeedRecreateSwapchain = false;

        uint32_t m_CurrentFrame = 0;
        uint32_t m_ImageIndex = 0;

        VkDescriptorPool m_ImGuiDescriptorPool = nullptr;

        static inline VulkanRendererBackend* m_Instance = nullptr;
    };

}    // namespace Vega
