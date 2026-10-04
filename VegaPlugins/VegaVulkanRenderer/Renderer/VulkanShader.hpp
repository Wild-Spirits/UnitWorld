#pragma once

#include "Vega/Renderer/Shader.hpp"
#include "VulkanRenderBuffer.hpp"
#include "VulkanSampler.hpp"
#include "VulkanTexture.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    struct VulkanDescriptorSetConfig
    {
        std::vector<VkDescriptorSetLayoutBinding> Bindings;
    };

    enum class VulkanPrimitiveTopologyTypeBase : uint32_t
    {
        kPoint = 0U,
        kLine,
        kTriangle,
    };

    struct VulkanPipeline
    {
        VulkanPrimitiveTopologyTypeBase TopologyTypeBase;
        VkPipeline Handle;
        VkPipelineLayout Layout;
        PrimitiveTopologyTypes SupportedTopologyTypes;
    };

    struct Range
    {
        size_t Offset;
        size_t Size;
    };

    struct VulkanPiplineConfig
    {
        std::string Name;
        uint32_t Stride;
        std::vector<VkVertexInputAttributeDescription> Attributes;
        std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
        std::vector<VkPipelineShaderStageCreateInfo> Stages;
        VkViewport Viewport;
        VkRect2D Scissor;
        FaceCullMode CullMode;
        ShaderFlags Flags;
        std::vector<Range> PushConstantRanges;
        PrimitiveTopologyTypes TopologyTypes;
        RendererWinding Winding;

        std::vector<VkFormat> ColorAttachmentFormats;
        VkFormat DepthAttachmentFormat;
        VkFormat StencilAttachmentFormat;
    };

    struct VulkanShaderStage
    {
        VkShaderModuleCreateInfo CreateInfo;
        VkShaderModule Handle;
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
    };

    struct VulkanShaderFrequencyInfo
    {
        size_t UboSize = 0;
        size_t UboStride = 0;
        // Offset of the frequency block inside one frame slot of the shader uniform buffer
        size_t UboOffset = 0;

        std::vector<size_t> UboIndices;
        std::vector<size_t> TextureIndices;
        std::vector<size_t> SamplerIndices;

        // Indexed same as frequency uniforms in ShaderConfig
        std::vector<uint32_t> UniformBindings;
        // Offset of uniform inside UBO block, only for non texture/sampler uniforms
        std::vector<size_t> UniformOffsets;
    };

    struct VulkanShaderFrequencyState
    {
        // CPU copy of UBO block, uploaded to the current frame slot on ApplyFrequency
        std::vector<uint8_t> UboData;
        // Keyed by binding index
        std::unordered_map<size_t, Ref<VulkanTexture>> UniformTextures;
        std::unordered_map<size_t, Ref<VulkanSampler>> UniformSamplers;

        // One per frame in flight
        std::vector<VkDescriptorSet> DescriptorSets;
    };

    /**
     * @brief VulkanShader class
     *
     * This class represents a shader in the Vulkan rendering backend.
     * It inherits from the Shader class and provides Vulkan-specific functionality.
     */
    class VulkanShader : public Shader
    {
    public:
        void Create(const ShaderConfig& _ShaderConfig,
                    const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) override;

        void Initialize() override;
        void OnDetach() override;

        bool Bind() override;

        void SetUniformBufferData(std::string_view _Name, const void* _Data, size_t _Size,
                                  ShaderUpdateFrequency _Frequency) override;

        void SetUniformTexture(std::string_view _Name, Ref<class Texture> _Texture,
                               ShaderUpdateFrequency _Frequency) override;

        void SetUniformSampler(std::string_view _Name, Ref<Sampler> _Sampler,
                               ShaderUpdateFrequency _Frequency) override;

        void BindFrequency(ShaderUpdateFrequency _Frequency) override;
        void ApplyFrequency(ShaderUpdateFrequency _Frequency) override;

    protected:
        void PrepareShaderData();

        VulkanDescriptorSetConfig SetupDescriptorSetConfigAndFrequency(bool _IsNeedDoUniformBuffers,
                                                                       const std::vector<ShaderUniform>& _Uniforms,
                                                                       VulkanShaderFrequencyInfo& _OutFrequencyInfo);

        void SetupFrequencyState(bool _IsNeedDoUniformBuffers, ShaderUpdateFrequency _Frequency);

        void CreateUniformBuffer();
        void DestroyUniformBuffer();

        bool CreateModulesAndPipelines();

        VkCullModeFlags GetVkCullMode(FaceCullMode _CullMode) const;
        VkFrontFace GetVkFrontFace(RendererWinding _Winding) const;
        VkPrimitiveTopology GetVkPrimitiveTopology(PrimitiveTopologyTypes _TopologyTypes) const;

        bool CreateGraphicsPipeline(const VulkanPiplineConfig& _PipelineConfig, VulkanPipeline& _OutPipeline);
        bool DestroyGraphicsPipeline(VulkanPipeline& _Pipeline);

        std::optional<VulkanShaderStage> CreateShaderModule(const ShaderStageConfig& _ShaderStageConfig);

        void BindPipeline(VkCommandBuffer _CommandBuffer, VkPipelineBindPoint _BindPoint,
                          const VulkanPipeline& _Pipeline);

        size_t GetUniformSamplerCount(const std::vector<ShaderUniform>& _Uniforms) const;
        size_t GetUniformTextureCount(const std::vector<ShaderUniform>& _Uniforms) const;
        size_t GetUniformBufferCount(const std::vector<ShaderUniform>& _Uniforms) const;

        const std::vector<ShaderUniform>& GetShaderUniformsForFrequency(ShaderUpdateFrequency _Frequency) const;

        const VulkanShaderFrequencyInfo&
        GetVulkanShaderFrequencyInfoForFrequency(ShaderUpdateFrequency _Frequency) const;

        VulkanShaderFrequencyState& GetVulkanShaderFrequencyStateForFrequency(ShaderUpdateFrequency _Frequency);

        const VulkanDescriptorSetConfig&
        GetVulkanDescriptorSetConfigForFrequency(ShaderUpdateFrequency _Frequency) const;

        size_t GetVulkanDescriptorSetConfigIndexForFrequency(ShaderUpdateFrequency _Frequency) const;

    protected:
        ShaderConfig m_ShaderConfig;
        std::vector<ShaderStageConfig> m_ShaderStageConfigs;
        std::vector<VulkanShaderStage> m_ShaderStages;

        uint8_t m_LocalPushConstantsBlock[128] = { 0 };

        std::vector<VulkanDescriptorSetConfig> m_DescriptorSetConfigs;
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

        size_t m_MaxDescriptorSetCount;

        std::vector<VkDescriptorPoolSize> m_PoolSizes;

        std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;

        VkDescriptorPool m_DescriptorPool;

        std::vector<VulkanPipeline> m_Pipelines;
        std::vector<VulkanPipeline> m_WireframesPipelines;

        size_t m_BoundPipelineIndex;
        VkPrimitiveTopology m_CurentTopology;

        size_t m_RequiredUboAlignment;

        // Per-frame and per-group UBO blocks, one slot per frame in flight: [frame 0: perFrame|perGroup][frame 1: ...]
        Ref<VulkanRenderBuffer> m_UniformBuffer = nullptr;
        uint8_t* m_MappedUniformBuffer = nullptr;
        size_t m_UniformBufferFrameStride = 0;

        VulkanShaderFrequencyInfo m_PerFrameInfo;
        VulkanShaderFrequencyState m_PerFrameState;
        size_t m_VulkanDescriptorSetConfigIndexPerFrame = 0;

        VulkanShaderFrequencyInfo m_PerGroupInfo;
        VulkanShaderFrequencyState m_PerGroupState;
        size_t m_VulkanDescriptorSetConfigIndexPerGroup = 0;

        VulkanShaderFrequencyInfo m_PerDrawInfo;
        VulkanShaderFrequencyState m_PerDrawState;
        size_t m_VulkanDescriptorSetConfigIndexPerDraw = 0;
    };

}    // namespace Vega
