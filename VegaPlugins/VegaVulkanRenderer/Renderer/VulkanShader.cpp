#include "VulkanShader.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Assert.hpp"
#include "Vega/Utils/magic_enum.hpp"

#include "Utils/VulkanUtils.hpp"
#include "Vega/Renderer/Shader.hpp"
#include "Vega/Utils/Log.hpp"
#include "VulkanBase.hpp"
#include "VulkanRendererBackend.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <limits>

#include <glm/glm.hpp>
#include <shaderc/shaderc.h>
#include <shaderc/status.h>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    static std::vector<char> ReadFile(const std::string& _Filename);

    static VkFormat ShaderAttributeTypeToVkFormat(ShaderAttributeType _Type);

    void VulkanShader::Create(const ShaderConfig& _ShaderConfig,
                              const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs)
    {
        m_ShaderConfig = _ShaderConfig;
        m_ShaderStageConfigs = _ShaderStageConfigs;

        PrepareShaderData();
    }

    void VulkanShader::Initialize()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VulkanDeviceWrapper deviceWrapper = rendererBackend->GetVkDeviceWrapper();
        VkDevice logicalDevice = deviceWrapper.GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        bool isNeedWireframe = (m_ShaderConfig.Flags & ShaderFlagBits::kWireframe) != 0;
        if (deviceWrapper.GetPhysicalDeviceFeatures().fillModeNonSolid)
        {
            VEGA_CORE_WARN("Vulkan does not support wireframe mode, disabling it.");
            isNeedWireframe = false;
        }

        uint32_t offset = 0;
        for (ShaderAttributeType attributeType : m_ShaderConfig.Attributes)
        {
            m_AttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
                .location = static_cast<uint32_t>(m_AttributeDescriptions.size()),
                .binding = 0,
                .format = ShaderAttributeTypeToVkFormat(attributeType),
                .offset = offset,
            });
            offset += ShaderDataTypeSize(attributeType);
        }

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = m_MaxDescriptorSetCount,
            .poolSizeCount = static_cast<uint32_t>(m_PoolSizes.size()),
            .pPoolSizes = m_PoolSizes.data(),
        };

#if defined(VK_USE_PLATFORM_MACOS_MVK)
        // NOTE: increase the per-stage descriptor samplers limit on macOS (maxPerStageDescriptorUpdateAfterBindSamplers
        // > maxPerStageDescriptorSamplers)
        poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
#endif

        VkResult createPoolResult = vkCreateDescriptorPool(logicalDevice, &poolInfo, vkAllocator, &m_DescriptorPool);
        if (!VulkanResultIsSuccess(createPoolResult))
        {
            VEGA_CORE_CRITICAL("Failed to create descriptor pool: {}", VulkanResultString(createPoolResult, true));
            VEGA_CORE_ASSERT(false, "Failed to create descriptor pool!");
        }

        for (size_t i = 0; i < m_DescriptorSets.size(); ++i)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = static_cast<uint32_t>(m_DescriptorSets[i].Bindings.size()),
                .pBindings = m_DescriptorSets[i].Bindings.data(),
            };

            VkResult createLayoutResult =
                vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, vkAllocator, &m_DescriptorSetLayouts[i]);

            if (!VulkanResultIsSuccess(createLayoutResult))
            {
                VEGA_CORE_CRITICAL("Failed to create descriptor set layout: {}",
                                   VulkanResultString(createLayoutResult, true));
                VEGA_CORE_ASSERT(false, "Failed to create descriptor set layout!");
            }
        }

        if (m_ShaderConfig.TopologyTypes & PrimitiveTopologyTypeBits::kPointList)
        {
            VulkanPipeline pipeline = {
                .TopologyTypeBase = VulkanPrimitiveTopologyTypeBase::kPoint,
                .Handle = nullptr,
                .Layout = nullptr,
                .SupportedTopologyTypes = PrimitiveTopologyTypeBits::kPointList,
            };
            m_Pipelines.emplace_back(pipeline);
            if (isNeedWireframe)
            {
                m_WireframesPipelines.emplace_back(pipeline);
            }
        }

        if (m_ShaderConfig.TopologyTypes & PrimitiveTopologyTypeBits::kLineList ||
            m_ShaderConfig.TopologyTypes & PrimitiveTopologyTypeBits::kLineStrip)
        {
            VulkanPipeline pipeline = {
                .TopologyTypeBase = VulkanPrimitiveTopologyTypeBase::kLine,
                .Handle = nullptr,
                .Layout = nullptr,
                .SupportedTopologyTypes = PrimitiveTopologyTypeBits::kLineList | PrimitiveTopologyTypeBits::kLineStrip,
            };
            m_Pipelines.emplace_back(pipeline);
            if (isNeedWireframe)
            {
                m_WireframesPipelines.emplace_back(pipeline);
            }
        }

        if (m_ShaderConfig.TopologyTypes & PrimitiveTopologyTypeBits::kTriangleList ||
            m_ShaderConfig.TopologyTypes & PrimitiveTopologyTypeBits::kTriangleStrip ||
            m_ShaderConfig.TopologyTypes & PrimitiveTopologyTypeBits::kTriangleFan)
        {
            VulkanPipeline pipeline = {
                .TopologyTypeBase = VulkanPrimitiveTopologyTypeBase::kTriangle,
                .Handle = nullptr,
                .Layout = nullptr,
                .SupportedTopologyTypes = PrimitiveTopologyTypeBits::kTriangleList |
                                          PrimitiveTopologyTypeBits::kTriangleStrip |
                                          PrimitiveTopologyTypeBits::kTriangleFan,
            };
            m_Pipelines.emplace_back(pipeline);
            if (isNeedWireframe)
            {
                m_WireframesPipelines.emplace_back(pipeline);
            }
        }

        if (!CreateModulesAndPipelines())
        {
            VEGA_CORE_ERROR("Failed initial load on shader {}. See logs for details.", m_ShaderConfig.Name);
            VEGA_CORE_ASSERT(false, "Failed initial load on shader");
            return;
        }

        m_BoundPipelineIndex = 0;
        bool pipelineFound = false;

        for (size_t i = 0; i < m_Pipelines.size(); ++i)
        {
            m_BoundPipelineIndex = i;
            m_CurentTopology = GetVkPrimitiveTopology(m_Pipelines[i].SupportedTopologyTypes);
            pipelineFound = true;
            break;
        }

        if (!pipelineFound)
        {
            VEGA_CORE_ERROR("No available topology classes are available, so a pipeline cannot be bound.");
            VEGA_CORE_ASSERT(false, "No available topology classes are available, so a pipeline cannot be bound.");
            return;
        }

        m_RequiredUboAlignment = deviceWrapper.GetMinUniformBufferOffsetAligment();

        // TODO: Continue here : implement ubo
    }

    void VulkanShader::Shutdown()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        for (size_t i = 0; i < m_DescriptorSets.size(); ++i)
        {
            vkDestroyDescriptorSetLayout(logicalDevice, m_DescriptorSetLayouts[i], vkAllocator);
        }
        m_DescriptorSets.clear();
        m_DescriptorSetLayouts.clear();

        // TODO: clear gloabal descriptor sets ?

        if (m_DescriptorPool)
        {
            vkDestroyDescriptorPool(logicalDevice, m_DescriptorPool, vkAllocator);
        }

        m_InstanceStates.clear();

        // TODO: clear Uniform buffer

        vkDeviceWaitIdle(logicalDevice);

        for (VulkanPipeline& pipeline : m_Pipelines)
        {
            DestroyGraphicsPipeline(pipeline);
        }
        for (VulkanPipeline& pipeline : m_WireframesPipelines)
        {
            DestroyGraphicsPipeline(pipeline);
        }
        m_Pipelines.clear();
        m_WireframesPipelines.clear();

        for (VulkanShaderStage& stage : m_ShaderStages)
        {
            vkDestroyShaderModule(logicalDevice, stage.Handle, vkAllocator);
        }

        m_ShaderStages.clear();
    }

    bool VulkanShader::Bind()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VulkanDeviceWrapper deviceWrapper = rendererBackend->GetVkDeviceWrapper();
        VkCommandBuffer commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();

        std::vector<VulkanPipeline>& pipelineArray =
            m_ShaderConfig.Flags & ShaderFlagBits::kWireframe ? m_WireframesPipelines : m_Pipelines;

        BindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineArray[m_BoundPipelineIndex]);

        // TODO: save bounded shader for optimizations

        if (deviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kNativeDynamicStateBit)
        {
            vkCmdSetPrimitiveTopology(commandBuffer, m_CurentTopology);
        }
        else if (deviceWrapper.GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit)
        {
            rendererBackend->GetVkContext().VkCmdSetPrimitiveTopologyEXT(commandBuffer, m_CurentTopology);
        }

        return true;
    }

    void VulkanShader::PrepareShaderData()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        m_LocalPushConstantsBlock.fill(0);

        bool isHasGlobalDescriptorSet =
            m_ShaderConfig.GlobalUnformCount > 0 || m_ShaderConfig.GlobalUniformSamplerCount > 0;
        bool isHasInstanceDescriptorSet =
            m_ShaderConfig.InstanceUniformCount > 0 || m_ShaderConfig.InstanceUniformSamplerCount > 0;

        m_DescriptorSets.reserve(2);
        m_DescriptorSetLayouts.reserve(2);
        if (isHasGlobalDescriptorSet)
        {
            m_DescriptorSets.emplace_back(VulkanDescriptorSetConfig {});
            m_DescriptorSetLayouts.emplace_back(VK_NULL_HANDLE);
        }
        if (isHasInstanceDescriptorSet)
        {
            m_DescriptorSets.emplace_back(VulkanDescriptorSetConfig {});
            m_DescriptorSetLayouts.emplace_back(VK_NULL_HANDLE);
        }

        uint32_t imageCount = static_cast<uint32_t>(rendererBackend->GetVkSwapchain().GetImagesCount());

        uint32_t maxSamplerCount =
            m_ShaderConfig.GlobalUniformSamplerCount * imageCount +
            m_ShaderConfig.MaxInstances * m_ShaderConfig.InstanceUniformSamplerCount * imageCount;
        uint32_t maxUboCount = imageCount + imageCount * m_ShaderConfig.MaxInstances;

        m_MaxDescriptorSetCount = maxUboCount + maxSamplerCount;

        m_PoolSizes.reserve(2);
        if (maxUboCount > 0)
        {
            m_PoolSizes.emplace_back(
                VkDescriptorPoolSize { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = maxUboCount });
        }
        if (maxSamplerCount > 0)
        {
            m_PoolSizes.emplace_back(VkDescriptorPoolSize { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                            .descriptorCount = maxSamplerCount });
        }

        if (isHasGlobalDescriptorSet)
        {
            VulkanDescriptorSetConfig& setConfig = m_DescriptorSets.front();
            uint32_t uboCount = m_ShaderConfig.GlobalUnformCount ? 1 : 0;
            setConfig.Bindings.reserve(uboCount + m_ShaderConfig.GlobalUniformSamplerCount);
            if (uboCount > 0)
            {
                setConfig.Bindings.emplace_back(VkDescriptorSetLayoutBinding {
                    .binding = static_cast<uint32_t>(setConfig.Bindings.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_ALL,
                });
            }

            setConfig.SamplerBindingIndexStart = uboCount;

            for (uint32_t i = 0; i < m_ShaderConfig.GlobalUniformSamplerCount; ++i)
            {
                const ShaderUniform& uniform = m_ShaderConfig.Uniforms[m_ShaderConfig.GlobalSamplerIndices[i]];
                setConfig.Bindings.emplace_back(VkDescriptorSetLayoutBinding {
                    .binding = static_cast<uint32_t>(setConfig.Bindings.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = glm::max(static_cast<uint32_t>(1), uniform.ArrayLength),
                    .stageFlags = VK_SHADER_STAGE_ALL,
                });
            }
        }

        if (isHasInstanceDescriptorSet)
        {
            VulkanDescriptorSetConfig& setConfig = m_DescriptorSets.back();
            uint32_t uboCount = m_ShaderConfig.InstanceUniformCount ? 1 : 0;
            setConfig.Bindings.reserve(uboCount + m_ShaderConfig.InstanceUniformSamplerCount);
            if (uboCount > 0)
            {
                setConfig.Bindings.emplace_back(VkDescriptorSetLayoutBinding {
                    .binding = static_cast<uint32_t>(setConfig.Bindings.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_ALL,
                });
            }

            setConfig.SamplerBindingIndexStart = uboCount;

            for (uint32_t i = 0; i < m_ShaderConfig.InstanceUniformSamplerCount; ++i)
            {
                const ShaderUniform& uniform = m_ShaderConfig.Uniforms[m_ShaderConfig.InstanceSamplerIndices[i]];
                setConfig.Bindings.emplace_back(VkDescriptorSetLayoutBinding {
                    .binding = static_cast<uint32_t>(setConfig.Bindings.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = glm::max(static_cast<uint32_t>(1), uniform.ArrayLength),
                    .stageFlags = VK_SHADER_STAGE_ALL,
                });
            }
        }

        m_GlobalUboDescriptorStates.Generations.resize(imageCount);
        m_GlobalUboDescriptorStates.Ids.resize(imageCount);
        m_GlobalUboDescriptorStates.FrameNumbers.resize(imageCount);
        for (size_t i = 0; i < imageCount; ++i)
        {
            m_GlobalUboDescriptorStates.Generations[i] = std::numeric_limits<uint32_t>::max();
            m_GlobalUboDescriptorStates.Ids[i] = std::numeric_limits<uint32_t>::max();
            m_GlobalUboDescriptorStates.FrameNumbers[i] = std::numeric_limits<uint64_t>::max();
        }

        m_InstanceStates.resize(m_ShaderConfig.MaxInstances);
        for (size_t i = 0; i < m_ShaderConfig.MaxInstances; ++i)
        {
            m_InstanceStates[i].Id = std::numeric_limits<uint32_t>::max();
        }
    }

    bool VulkanShader::CreateModulesAndPipelines()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        bool hasError = false;

        bool isNeedWireframe = (m_ShaderConfig.Flags & ShaderFlagBits::kWireframe) != 0;

        std::vector<VulkanPipeline> newPipelines;
        std::vector<VulkanPipeline> newWireframePipelines;

        std::vector<VulkanShaderStage> newStages;
        newStages.reserve(m_ShaderStageConfigs.size());

        for (size_t i = 0; i < m_ShaderStageConfigs.size(); ++i)
        {
            std::optional<VulkanShaderStage> vkStage = CreateShaderModule(m_ShaderStageConfigs[i]);
            if (vkStage.has_value())
            {
                newStages.push_back(vkStage.value());
                continue;
            }

            VEGA_CORE_ERROR("Failed to create shader module for stage: {}",
                            ShaderStageTypeToString(m_ShaderStageConfigs[i].Type));
            hasError = true;
            break;
        }

        if (hasError)
        {
            // TODO: Implement proper error handling ?
            return false;
        }

        Ref<Window> window = Application::Get().GetWindow();

        VEGA_CORE_WARN("ww: {} {}", static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight()));

        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(window->GetWidth()),
            .height = static_cast<float>(window->GetHeight()),
            .minDepth = 0.0f,
            .maxDepth = 0.0f,
        };

        VkRect2D scissor = {
            .offset = {                      .x = 0,                        .y = 0 },
            .extent = { .width = window->GetWidth(), .height = window->GetHeight() },
        };

        std::vector<VkPipelineShaderStageCreateInfo> stagesCreateInfo;
        stagesCreateInfo.reserve(newStages.size());
        std::transform(newStages.begin(), newStages.end(), std::back_inserter(stagesCreateInfo),
                       [](const VulkanShaderStage& stage) { return stage.ShaderStageCreateInfo; });

        for (size_t i = 0; i < m_Pipelines.size(); ++i)
        {
            newPipelines.emplace_back(VulkanPipeline {
                .SupportedTopologyTypes = m_Pipelines[i].SupportedTopologyTypes,
            });
            if (isNeedWireframe)
            {
                newWireframePipelines.emplace_back(VulkanPipeline {
                    .SupportedTopologyTypes = m_WireframesPipelines[i].SupportedTopologyTypes,
                });
            }

            bool isColorFlagSet = (m_ShaderConfig.Flags & ShaderFlagBits::kColorRead) ||
                                  (m_ShaderConfig.Flags & ShaderFlagBits::kColorWrite);

            VkFormat colorFormat = rendererBackend->GetVkSwapchain().GetImageFormat();

            bool isDepthOrStencilFlagSet = (m_ShaderConfig.Flags & ShaderFlagBits::kDepthTest) ||
                                           (m_ShaderConfig.Flags & ShaderFlagBits::kDepthWrite) ||
                                           (m_ShaderConfig.Flags & ShaderFlagBits::kStencilTest) ||
                                           (m_ShaderConfig.Flags & ShaderFlagBits::kStencilWrite);

            VkFormat depthFormat = rendererBackend->GetVkDeviceWrapper().GetDepthFormat();

            // TODO: Remove hardcoded value
            const uint32_t localUboStride = 128;

            VulkanPiplineConfig pipelineConfig = {
                .Name = m_ShaderConfig.Name,
                .Stride = m_ShaderConfig.GetAttibutesStride(),
                .Attributes = m_AttributeDescriptions,
                .DescriptorSetLayouts = m_DescriptorSetLayouts,
                .Stages = stagesCreateInfo,
                .Viewport = viewport,
                .Scissor = scissor,
                .CullMode = m_ShaderConfig.CullMode,
                .Flags = m_ShaderConfig.Flags & ~ShaderFlagBits::kWireframe,
                .PushConstantRanges = { Range { .Offset = 0, .Size = localUboStride } },
                .TopologyTypes = m_ShaderConfig.TopologyTypes,
                .ColorAttachmentFormats =
                    isColorFlagSet ? std::vector<VkFormat> { colorFormat } : std::vector<VkFormat> {},
                .DepthAttachmentFormat = isDepthOrStencilFlagSet ? depthFormat : VK_FORMAT_UNDEFINED,
                .StencilAttachmentFormat = isDepthOrStencilFlagSet ? depthFormat : VK_FORMAT_UNDEFINED,
            };

            bool pipelineResult = CreateGraphicsPipeline(pipelineConfig, newPipelines[i]);

            if (pipelineResult && isNeedWireframe)
            {
                pipelineConfig.Flags |= ShaderFlagBits::kWireframe;
                pipelineResult = CreateGraphicsPipeline(pipelineConfig, newWireframePipelines[i]);
            }

            if (!pipelineResult)
            {
                VEGA_CORE_ERROR("Failed to load graphics pipeline for shader: {}.", m_ShaderConfig.Name);
                hasError = true;
                break;
            }
        }

        if (hasError)
        {
            for (VulkanPipeline& pipeline : newPipelines)
            {
                DestroyGraphicsPipeline(pipeline);
            }
            for (VulkanPipeline& pipeline : newWireframePipelines)
            {
                DestroyGraphicsPipeline(pipeline);
            }
            for (VulkanShaderStage& stage : newStages)
            {
                vkDestroyShaderModule(logicalDevice, stage.Handle, vkAllocator);
            }

            return false;
        }

        vkDeviceWaitIdle(logicalDevice);

        for (VulkanPipeline& pipeline : m_Pipelines)
        {
            DestroyGraphicsPipeline(pipeline);
        }
        m_Pipelines = std::move(newPipelines);

        for (VulkanPipeline& pipeline : m_WireframesPipelines)
        {
            DestroyGraphicsPipeline(pipeline);
        }
        m_WireframesPipelines = std::move(newWireframePipelines);

        for (VulkanShaderStage& stage : m_ShaderStages)
        {
            vkDestroyShaderModule(logicalDevice, stage.Handle, vkAllocator);
        }
        m_ShaderStages = std::move(newStages);

        return true;
    }

    VkCullModeFlags VulkanShader::GetVkCullMode(FaceCullMode _CullMode) const
    {
        switch (_CullMode)
        {
            case FaceCullMode::kNone: return VK_CULL_MODE_NONE;
            case FaceCullMode::kFront: return VK_CULL_MODE_FRONT_BIT;
            case FaceCullMode::kBack: return VK_CULL_MODE_BACK_BIT;
            case FaceCullMode::kFrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
        }

        VEGA_CORE_WARN("VulkanShader::GetVkCullMode. Unknown cull mode!");
        return VK_CULL_MODE_NONE;
    }

    VkFrontFace VulkanShader::GetVkFrontFace(RendererWinding _Winding) const
    {
        switch (_Winding)
        {
            case RendererWinding::kRendererWindingClockwise: return VK_FRONT_FACE_CLOCKWISE;
            case RendererWinding::kRendererWindingCounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        VEGA_CORE_WARN("VulkanShader::GetVkFrontFace. Unknown winding!");
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    VkPrimitiveTopology VulkanShader::GetVkPrimitiveTopology(PrimitiveTopologyTypes _TopologyTypes) const
    {
        for (PrimitiveTopologyTypeBits::PrimitiveTopologyTypeBits topologyType :
             magic_enum::enum_values<PrimitiveTopologyTypeBits::PrimitiveTopologyTypeBits>())
        {
            VEGA_CORE_WARN("topologyType: {}", static_cast<uint32_t>(topologyType));
            if (_TopologyTypes & topologyType)
            {
                switch (topologyType)
                {
                    case PrimitiveTopologyTypeBits::kNone:
                        VEGA_CORE_WARN(
                            "VulkanShader::GetVkPrimitiveTopology. PrimitiveTopologyTypeBits::kNone selected!");
                        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    case PrimitiveTopologyTypeBits::kTriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    case PrimitiveTopologyTypeBits::kTriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                    case PrimitiveTopologyTypeBits::kTriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                    case PrimitiveTopologyTypeBits::kLineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                    case PrimitiveTopologyTypeBits::kLineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                    case PrimitiveTopologyTypeBits::kPointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                }
            }
        }

        VEGA_CORE_WARN("VulkanShader::GetVkPrimitiveTopology. Unknown primitive topology type!");
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    bool VulkanShader::CreateGraphicsPipeline(const VulkanPiplineConfig& _PipelineConfig, VulkanPipeline& _OutPipeline)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &_PipelineConfig.Viewport,
            .scissorCount = 1,
            .pScissors = &_PipelineConfig.Scissor,
        };

        VkPipelineRasterizationLineStateCreateInfoEXT lineRasterizationExtCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT,
            .lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT,
        };

        bool isHasLineSmooth = rendererBackend->GetVkDeviceWrapper().GetSupportFlags() &
                               VulkanDeviceSupportFlagBits::kLineSmoothRasterizationBit;

        bool isWireframe = _PipelineConfig.Flags & ShaderFlagBits::kWireframe;

        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = isHasLineSmooth ? &lineRasterizationExtCreateInfo : nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = isWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
            .cullMode = GetVkCullMode(_PipelineConfig.CullMode),
            .frontFace = GetVkFrontFace(_PipelineConfig.Winding),
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = 0,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        VkStencilOpState stencilOpState = {
            .failOp = VK_STENCIL_OP_ZERO,
            .passOp = VK_STENCIL_OP_REPLACE,
            .depthFailOp = VK_STENCIL_OP_ZERO,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .compareMask = 0xff,
            .writeMask = _PipelineConfig.Flags & ShaderFlagBits::kStencilWrite ? 0xffu : 0x00u,
            .reference = 1,
        };

        bool isDepthTest = _PipelineConfig.Flags & ShaderFlagBits::kDepthTest;
        bool isStencilTest = _PipelineConfig.Flags & ShaderFlagBits::kStencilTest;

        VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = isDepthTest ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = _PipelineConfig.Flags & ShaderFlagBits::kDepthWrite ? VK_TRUE : VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = isStencilTest ? VK_TRUE : VK_FALSE,
            .front = stencilOpState,
            .back = stencilOpState,
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachmentState,
        };

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        if ((rendererBackend->GetVkDeviceWrapper().GetSupportFlags() &
             VulkanDeviceSupportFlagBits::kNativeDynamicStateBit) ||
            (rendererBackend->GetVkDeviceWrapper().GetSupportFlags() & VulkanDeviceSupportFlagBits::kDynamicStateBit))
        {
            dynamicStates.insert(dynamicStates.end(), {
                                                          VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
                                                          VK_DYNAMIC_STATE_FRONT_FACE,
                                                          VK_DYNAMIC_STATE_STENCIL_OP,
                                                          VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT,
                                                          VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
                                                          VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
                                                          VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
                                                          VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
                                                          VK_DYNAMIC_STATE_STENCIL_REFERENCE,
                                                      });
            //   VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT,
            //   VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT,
        }

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data(),
        };

        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = _PipelineConfig.Stride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkPipelineVertexInputStateCreateInfo vertextInputCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = _PipelineConfig.Attributes.size() > 0 ? 1u : 0u,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(_PipelineConfig.Attributes.size()),
            .pVertexAttributeDescriptions = _PipelineConfig.Attributes.data(),
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = GetVkPrimitiveTopology(_OutPipeline.SupportedTopologyTypes),
            .primitiveRestartEnable = VK_FALSE,
        };

        std::vector<VkPushConstantRange> pushConstantRanges;
        if (_PipelineConfig.PushConstantRanges.size() > 32)
        {
            VEGA_CORE_ERROR("VulkanShader::CreateGraphicsPipeline: cannot have more than 32 push constant ranges. "
                            "Passed count: {}",
                            _PipelineConfig.PushConstantRanges.size());
            return false;
        }
        for (const Range& range : _PipelineConfig.PushConstantRanges)
        {
            pushConstantRanges.emplace_back(VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = static_cast<uint32_t>(range.Offset),
                .size = static_cast<uint32_t>(range.Size),
            });
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(_PipelineConfig.DescriptorSetLayouts.size()),
            .pSetLayouts = _PipelineConfig.DescriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(_PipelineConfig.PushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.data(),
        };

        VK_CHECK(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, vkAllocator, &_OutPipeline.Layout));

#ifdef _DEBUG
        std::string pipelineLayoutName = std::format("pipeline_layout_shader_{}", _PipelineConfig.Name);
        VK_SET_DEBUG_OBJECT_NAME(rendererBackend->GetVkContext().PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                                 VK_OBJECT_TYPE_PIPELINE_LAYOUT, _OutPipeline.Layout, pipelineLayoutName.data());
#endif

        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .colorAttachmentCount = static_cast<uint32_t>(_PipelineConfig.ColorAttachmentFormats.size()),
            .pColorAttachmentFormats = _PipelineConfig.ColorAttachmentFormats.data(),
            .depthAttachmentFormat = _PipelineConfig.DepthAttachmentFormat,
            .stencilAttachmentFormat = _PipelineConfig.StencilAttachmentFormat,
        };

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipelineRenderingCreateInfo,
            .stageCount = static_cast<uint32_t>(_PipelineConfig.Stages.size()),
            .pStages = _PipelineConfig.Stages.data(),
            .pVertexInputState = &vertextInputCreateInfo,
            .pInputAssemblyState = &inputAssemblyCreateInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportStateCreateInfo,
            .pRasterizationState = &rasterizerCreateInfo,
            .pMultisampleState = &multisamplingCreateInfo,
            .pDepthStencilState = isDepthTest || isStencilTest ? &depthStencilCreateInfo : nullptr,
            .pColorBlendState = &colorBlendStateCreateInfo,
            .pDynamicState = &dynamicStateCreateInfo,
            .layout = _OutPipeline.Layout,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        VkResult pipelineResult = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                                                            vkAllocator, &_OutPipeline.Handle);

#ifdef _DEBUG
        std::string pipelineName = std::format("pipeline_shader_{}", _PipelineConfig.Name);
        VK_SET_DEBUG_OBJECT_NAME(rendererBackend->GetVkContext().PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                                 VK_OBJECT_TYPE_PIPELINE, _OutPipeline.Handle, pipelineName.data());
#endif

        if (!VulkanResultIsSuccess(pipelineResult))
        {
            VEGA_CORE_ERROR("vkCreateGraphicsPipelines failed with {}.", VulkanResultString(pipelineResult, true));
            return false;
        }

        VEGA_CORE_TRACE("Graphics pipeline ({}) created!", _PipelineConfig.Name);
        return true;
    }

    bool VulkanShader::DestroyGraphicsPipeline(VulkanPipeline& _Pipeline)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        if (_Pipeline.Handle)
        {
            vkDestroyPipeline(logicalDevice, _Pipeline.Handle, vkAllocator);
            _Pipeline.Handle = VK_NULL_HANDLE;
        }

        if (_Pipeline.Layout)
        {
            vkDestroyPipelineLayout(logicalDevice, _Pipeline.Layout, vkAllocator);
            _Pipeline.Layout = VK_NULL_HANDLE;
        }

        return true;
    }

    std::optional<VulkanShaderStage> VulkanShader::CreateShaderModule(const ShaderStageConfig& _ShaderStageConfig)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VulkanContext context = rendererBackend->GetVkContext();

        shaderc_shader_kind shaderKind;
        VkShaderStageFlagBits stageFlag = VK_SHADER_STAGE_ALL;
        switch (_ShaderStageConfig.Type)
        {
            case ShaderStageConfig::ShaderStageType::kVertex:
                shaderKind = shaderc_glsl_default_vertex_shader;
                stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderStageConfig::ShaderStageType::kFragment:
                shaderKind = shaderc_glsl_default_fragment_shader;
                stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderStageConfig::ShaderStageType::kCompute:
                shaderKind = shaderc_glsl_default_compute_shader;
                stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            case ShaderStageConfig::ShaderStageType::kGeometry:
                shaderKind = shaderc_glsl_default_geometry_shader;
                stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            default:
                VEGA_CORE_ERROR("Unknown shader stage type: {}", ShaderStageTypeToString(_ShaderStageConfig.Type));
                return std::nullopt;
        }

        VEGA_CORE_TRACE("Compiling stage {} for shader: {}", ShaderStageTypeToString(_ShaderStageConfig.Type),
                        m_ShaderConfig.Name);

        std::vector<char> fileData = ReadFile(_ShaderStageConfig.Path);

        shaderc_compilation_result_t compilationResult =
            shaderc_compile_into_spv(context.ShaderCompiler, fileData.data(), fileData.size(), shaderKind,
                                     _ShaderStageConfig.Path.c_str(), "main", 0);

        if (!compilationResult)
        {
            VEGA_CORE_ERROR("An unknown error occurred while trying to compile the shader. Unable to process futher.");
            return std::nullopt;
        }

        shaderc_compilation_status status = shaderc_result_get_compilation_status(compilationResult);

        if (status != shaderc_compilation_status_success)
        {
            VEGA_CORE_ERROR("Error compiling shader with {} errors.", shaderc_result_get_num_errors(compilationResult));
            VEGA_CORE_ERROR("Error(s): \n \t {}", shaderc_result_get_error_message(compilationResult));

            shaderc_result_release(compilationResult);

            return std::nullopt;
        }

        VEGA_CORE_TRACE("Shader compiled successfully.");

        size_t warningCount = shaderc_result_get_num_warnings(compilationResult);
        if (warningCount)
        {
            VEGA_CORE_WARN("Warning compiling shader with {} warnings.", warningCount);
            // NOTE: Not sure this it the correct way to obtain warnings.
            VEGA_CORE_WARN("Warning(s): \n \t {}", shaderc_result_get_error_message(compilationResult));
        }

        const char* bytes = shaderc_result_get_bytes(compilationResult);
        size_t bytesLength = shaderc_result_get_length(compilationResult);

        std::ofstream outFile(std::format("{}.spv", _ShaderStageConfig.Path), std::ios::binary);
        if (outFile.is_open())
        {
            outFile.write(bytes, bytesLength);
            outFile.close();
        }
        else
        {
            VEGA_CORE_WARN("Failed to write SPIR-V binary to file: {}.spv", _ShaderStageConfig.Path);
        }

        VulkanShaderStage resStage {};
        resStage.CreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = bytesLength,
            .pCode = reinterpret_cast<const uint32_t*>(bytes),
        };

        VK_CHECK(vkCreateShaderModule(rendererBackend->GetVkDeviceWrapper().GetLogicalDevice(), &resStage.CreateInfo,
                                      context.VkAllocator, &resStage.Handle));

        // NOTE: may need to use this in resStage.CreateInfo
        // std::shared_ptr<char*> code = std::make_shared<char*>(new char[bytesLength]);
        // std::memcpy(*code, bytes, bytesLength);

        shaderc_result_release(compilationResult);

        resStage.ShaderStageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stageFlag,
            .module = resStage.Handle,
            .pName = "main",
        };

        return resStage;
    }

    void VulkanShader::BindPipeline(VkCommandBuffer _CommandBuffer, VkPipelineBindPoint _BindPoint,
                                    const VulkanPipeline& _Pipeline)
    {
        vkCmdBindPipeline(_CommandBuffer, _BindPoint, _Pipeline.Handle);
    }

    std::vector<char> ReadFile(const std::string& _Filename)
    {
        std::ifstream file(_Filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            VEGA_CORE_CRITICAL("Failed to open file: {}", _Filename);
            VEGA_CORE_ASSERT(false, "Failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkFormat ShaderAttributeTypeToVkFormat(ShaderAttributeType _Type)
    {
        switch (_Type)
        {
            case ShaderAttributeType::kFloat: return VK_FORMAT_R32_SFLOAT;
            case ShaderAttributeType::kFloat2: return VK_FORMAT_R32G32_SFLOAT;
            case ShaderAttributeType::kFloat3: return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderAttributeType::kFloat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            // TODO: Add support for matrix types
            // case ShaderAttributeType::kMat3: return VK_FORMAT_R32G32B32_SFLOAT;
            // case ShaderAttributeType::kMat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ShaderAttributeType::kInt8: return VK_FORMAT_R8_SINT;
            case ShaderAttributeType::kUint8: return VK_FORMAT_R8_UINT;
            case ShaderAttributeType::kInt16: return VK_FORMAT_R16_SINT;
            case ShaderAttributeType::kUint16: return VK_FORMAT_R16_UINT;
            case ShaderAttributeType::kInt32: return VK_FORMAT_R32_SINT;
            case ShaderAttributeType::kUint32: return VK_FORMAT_R32_UINT;
        }

        VEGA_CORE_ASSERT(false, "Unsupported shader attribute type!");
        return VK_FORMAT_UNDEFINED;
    }

}    // namespace Vega
