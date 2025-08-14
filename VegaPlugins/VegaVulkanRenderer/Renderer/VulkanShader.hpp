#pragma once

#include "Vega/Renderer/Shader.hpp"

#include <array>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    struct VulkanDescriptorState
    {
        std::vector<uint32_t> Generations;
        std::vector<uint32_t> Ids;
        std::vector<uint64_t> FrameNumbers;
    };

    struct VulkanUniformSamplerState
    {
        std::vector<ShaderUniform> Uniforms;

        // TODO: Texture maps ???

        VulkanDescriptorState DescriptorState;
    };

    struct VulkanShaderInstanceState
    {
        uint32_t Id;
        size_t Offset;

        std::vector<VkDescriptorSet> DescriptorSets;
        std::vector<VulkanDescriptorState> UboDescriptorState;
        std::vector<VulkanUniformSamplerState> SamplerUniforms;
    };

    struct VulkanDescriptorSetConfig
    {
        std::vector<VkDescriptorSetLayoutBinding> Bindings;
        uint32_t SamplerBindingIndexStart;
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

        bool Bind() override;

    protected:
        void PrepareShaderData();

        bool CreateModulesAndPipelines();

        VkCullModeFlags GetVkCullMode(FaceCullMode _CullMode) const;
        VkFrontFace GetVkFrontFace(RendererWinding _Winding) const;
        VkPrimitiveTopology GetVkPrimitiveTopology(PrimitiveTopologyTypes _TopologyTypes) const;

        bool CreateGraphicsPipline(const VulkanPiplineConfig& _PipelineConfig, VulkanPipeline& _OutPipeline);
        bool DestroyGraphicsPipline(VulkanPipeline& _OutPipeline);

        std::optional<VulkanShaderStage> CreateShaderModule(const ShaderStageConfig& _ShaderStageConfig);

        void BindPipeline(VkCommandBuffer _CommandBuffer, VkPipelineBindPoint _BindPoint,
                          const VulkanPipeline& _Pipeline);

    protected:
        ShaderConfig m_ShaderConfig;
        std::vector<ShaderStageConfig> m_ShaderStageConfigs;
        std::vector<VulkanShaderStage> m_ShaderStages;

        std::array<uint8_t, 128> m_LocalPushConstantsBlock;

        std::vector<VulkanDescriptorSetConfig> m_DescriptorSets;
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

        uint32_t m_MaxDescriptorSetCount;

        std::vector<VkDescriptorPoolSize> m_PoolSizes;

        VulkanDescriptorState m_GlobalUboDescriptorStates;

        std::vector<VulkanShaderInstanceState> m_InstanceStates;

        std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;

        VkDescriptorPool m_DescriptorPool;

        std::vector<VulkanPipeline> m_Pipelines;
        std::vector<VulkanPipeline> m_WireframesPipelines;

        size_t m_BoundPipelineIndex;
        VkPrimitiveTopology m_CurentTopology;

        uint32_t m_RequiredUboAlignment;
    };

}    // namespace Vega
