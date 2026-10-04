#include "VulkanShader.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Assert.hpp"
#include "Vega/Core/Base.hpp"
#include "Vega/Utils/magic_enum.hpp"

#include "Utils/VulkanUtils.hpp"
#include "Vega/Renderer/Shader.hpp"
#include "Vega/Utils/Log.hpp"
#include "VulkanBase.hpp"
#include "VulkanRendererBackend.hpp"
#include "VulkanTexture.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>

#include <glm/glm.hpp>
#include <shaderc/shaderc.h>
#include <shaderc/status.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    static std::vector<char> ReadFile(const std::string& _Filename);

    static VkFormat ShaderAttributeTypeToVkFormat(ShaderAttributeType _Type);

    static constexpr size_t GetAligned(size_t _Operand, size_t _Granularity)
    {
        return ((_Operand + (_Granularity - 1)) & ~(_Granularity - 1));
    }

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
            offset += GetShaderAttributeTypeSize(attributeType);
        }

        if (m_MaxDescriptorSetCount > 0)
        {
            // TODO: Should create pool of pools and handle overflow of pool with descriptors to create new
            VkDescriptorPoolCreateInfo poolInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .maxSets = static_cast<uint32_t>(m_MaxDescriptorSetCount),
                .poolSizeCount = static_cast<uint32_t>(m_PoolSizes.size()),
                .pPoolSizes = m_PoolSizes.data(),
            };

#if defined(VK_USE_PLATFORM_MACOS_MVK)
            // NOTE: increase the per-stage descriptor samplers limit on macOS
            // (maxPerStageDescriptorUpdateAfterBindSamplers > maxPerStageDescriptorSamplers)
            poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
#endif

            VkResult createPoolResult =
                vkCreateDescriptorPool(logicalDevice, &poolInfo, vkAllocator, &m_DescriptorPool);
            if (!VulkanResultIsSuccess(createPoolResult))
            {
                VEGA_CORE_CRITICAL("Failed to create descriptor pool: {}", VulkanResultString(createPoolResult, true));
                VEGA_CORE_ASSERT(false, "Failed to create descriptor pool!");
            }

            VK_SET_DEBUG_OBJECT_NAME(rendererBackend->GetVkContext().PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                                     VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_DescriptorPool,
                                     std::format("descriptor_pool_{}", m_ShaderConfig.Name).c_str());

            for (size_t i = 0; i < m_DescriptorSetConfigs.size(); ++i)
            {
                VkDescriptorSetLayoutCreateInfo layoutInfo = {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .bindingCount = static_cast<uint32_t>(m_DescriptorSetConfigs[i].Bindings.size()),
                    .pBindings = m_DescriptorSetConfigs[i].Bindings.data(),
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

        CreateUniformBuffer();

        // NOTE: Per-draw data goes through push constants, so it has no UBO
        SetupFrequencyState(true, ShaderUpdateFrequency::kPerFrame);
        SetupFrequencyState(true, ShaderUpdateFrequency::kPerGroup);
        SetupFrequencyState(false, ShaderUpdateFrequency::kPerDraw);
    }

    void VulkanShader::OnDetach()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        for (size_t i = 0; i < m_DescriptorSetLayouts.size(); ++i)
        {
            vkDestroyDescriptorSetLayout(logicalDevice, m_DescriptorSetLayouts[i], vkAllocator);
        }
        m_DescriptorSetLayouts.clear();
        m_DescriptorSetConfigs.clear();

        // TODO: clear gloabal descriptor sets ?

        if (m_DescriptorPool)
        {
            vkDestroyDescriptorPool(logicalDevice, m_DescriptorPool, vkAllocator);
        }

        // TODO: clear FrequencyInfo and FrequencyState

        DestroyUniformBuffer();

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

    void VulkanShader::SetUniformBufferData(std::string_view _Name, const void* _Data, size_t _Size,
                                            ShaderUpdateFrequency _Frequency)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkCommandBuffer commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();

        std::vector<VulkanPipeline>& pipelineArray =
            m_ShaderConfig.Flags & ShaderFlagBits::kWireframe ? m_WireframesPipelines : m_Pipelines;

        if (_Frequency == ShaderUpdateFrequency::kPerDraw)
        {
            std::memcpy(m_LocalPushConstantsBlock, _Data, _Size);
            vkCmdPushConstants(commandBuffer, pipelineArray[m_BoundPipelineIndex].Layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128,
                               m_LocalPushConstantsBlock);
            return;
        }

        const VulkanShaderFrequencyInfo& frequencyInfo = GetVulkanShaderFrequencyInfoForFrequency(_Frequency);
        VulkanShaderFrequencyState& frequencyState = GetVulkanShaderFrequencyStateForFrequency(_Frequency);
        const std::vector<ShaderUniform>& frequencyUniforms = GetShaderUniformsForFrequency(_Frequency);

        const auto it = std::find_if(frequencyUniforms.begin(), frequencyUniforms.end(),
                                     [&](const ShaderUniform& u) { return u.Name == _Name; });
        if (it == frequencyUniforms.end())
        {
            VEGA_CORE_WARN("SetUniformBufferData: uniform '{}' not found in shader '{}'", _Name, m_ShaderConfig.Name);
            return;
        }
        if (IsShaderUniformTypeSampler(it->Type) || IsShaderUniformTypeTexture(it->Type))
        {
            VEGA_CORE_WARN("SetUniformBufferData: uniform '{}' in shader '{}' is not a buffer uniform", _Name,
                           m_ShaderConfig.Name);
            return;
        }

        size_t uniformSize = it->Size * glm::max(1u, it->ArrayLength);
        if (_Size > uniformSize)
        {
            VEGA_CORE_WARN("SetUniformBufferData: data size {} is bigger than uniform '{}' size {} in shader '{}'",
                           _Size, _Name, uniformSize, m_ShaderConfig.Name);
            return;
        }

        size_t uniformIndex = static_cast<size_t>(std::distance(frequencyUniforms.begin(), it));
        size_t uniformOffset = frequencyInfo.UniformOffsets[uniformIndex];
        VEGA_CORE_ASSERT(uniformOffset + _Size <= frequencyState.UboData.size(), "Uniform is out of UBO bounds!");

        // NOTE: Uploaded to GPU memory on ApplyFrequency
        std::memcpy(frequencyState.UboData.data() + uniformOffset, _Data, _Size);
    }

    void VulkanShader::SetUniformTexture(std::string_view _Name, Ref<class Texture> _Texture,
                                         ShaderUpdateFrequency _Frequency)
    {
        VulkanShaderFrequencyState& frequencyState = GetVulkanShaderFrequencyStateForFrequency(_Frequency);
        const std::vector<ShaderUniform>& frequencyUniforms = GetShaderUniformsForFrequency(_Frequency);

        const auto it = std::find_if(frequencyUniforms.begin(), frequencyUniforms.end(),
                                     [&](const ShaderUniform& u) { return u.Name == _Name; });
        if (it == frequencyUniforms.end())
        {
            VEGA_CORE_WARN("SetUniformTexture: uniform '{}' not found in shader '{}'", _Name, m_ShaderConfig.Name);
            return;
        }
        size_t uniformIndex = static_cast<size_t>(std::distance(frequencyUniforms.begin(), it));
        size_t bindingIndex = GetVulkanShaderFrequencyInfoForFrequency(_Frequency).UniformBindings[uniformIndex];
        frequencyState.UniformTextures[bindingIndex] = StaticRefCast<VulkanTexture>(_Texture);
    }

    void VulkanShader::SetUniformSampler(std::string_view _Name, Ref<Sampler> _Sampler,
                                         ShaderUpdateFrequency _Frequency)
    {
        VulkanShaderFrequencyState& frequencyState = GetVulkanShaderFrequencyStateForFrequency(_Frequency);
        const std::vector<ShaderUniform>& frequencyUniforms = GetShaderUniformsForFrequency(_Frequency);

        const auto it = std::find_if(frequencyUniforms.begin(), frequencyUniforms.end(),
                                     [&](const ShaderUniform& u) { return u.Name == _Name; });
        if (it == frequencyUniforms.end())
        {
            VEGA_CORE_WARN("SetUniformSampler: uniform '{}' not found in shader '{}'", _Name, m_ShaderConfig.Name);
            return;
        }
        size_t uniformIndex = static_cast<size_t>(std::distance(frequencyUniforms.begin(), it));
        size_t bindingIndex = GetVulkanShaderFrequencyInfoForFrequency(_Frequency).UniformBindings[uniformIndex];
        frequencyState.UniformSamplers[bindingIndex] = StaticRefCast<VulkanSampler>(_Sampler);
    }

    void VulkanShader::BindFrequency(ShaderUpdateFrequency _Frequency)
    {
        // TODO: implement binding frequency
    }

    void VulkanShader::ApplyFrequency(ShaderUpdateFrequency _Frequency)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        const VulkanDescriptorSetConfig& descriptorSetConfig = GetVulkanDescriptorSetConfigForFrequency(_Frequency);
        const VulkanShaderFrequencyInfo& frequencyInfo = GetVulkanShaderFrequencyInfoForFrequency(_Frequency);
        const VulkanShaderFrequencyState& frequencyState = GetVulkanShaderFrequencyStateForFrequency(_Frequency);
        const std::vector<ShaderUniform>& frequencyUniforms = GetShaderUniformsForFrequency(_Frequency);
        const size_t descriptorSetIndex = GetVulkanDescriptorSetConfigIndexForFrequency(_Frequency);

        VEGA_CORE_WARN("VulkanShader::ApplyFrequency GetCurrentImageIndex: {}",
                       rendererBackend->GetCurrentImageIndex());
        VEGA_CORE_WARN("VulkanShader::ApplyFrequency GetCurrentFrameIndex: {}",
                       rendererBackend->GetCurrentFrameIndex());

        const uint32_t frameIndex = rendererBackend->GetCurrentFrameIndex();

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.reserve(descriptorSetConfig.Bindings.size());

        if (frequencyInfo.UboSize > 0)
        {
            // NOTE: The in-flight fence of this frame is already waited, so its slot is not read by GPU. UBO descriptor
            // points to this slot since SetupFrequencyState, so only data is updated here
            uint8_t* frameSlot = m_MappedUniformBuffer + frameIndex * m_UniformBufferFrameStride;
            std::memcpy(frameSlot + frequencyInfo.UboOffset, frequencyState.UboData.data(), frequencyInfo.UboSize);
        }

        size_t samplerAndTextureCount = frequencyInfo.SamplerIndices.size() + frequencyInfo.TextureIndices.size();
        std::vector<std::vector<VkDescriptorImageInfo>> imageInfos;
        imageInfos.reserve(samplerAndTextureCount);
        if (samplerAndTextureCount > 0)
        {
            // TODO: make this alloacation more efficient (may store in frequency info or state)
            for (size_t textureIndex : frequencyInfo.TextureIndices)
            {
                size_t descriptorCount = descriptorSetConfig.Bindings[textureIndex].descriptorCount;
                std::vector<VkDescriptorImageInfo> currentImageInfos(descriptorCount);
                // currentImageInfos.reserve(descriptorCount);
                for (VkDescriptorImageInfo& imageInfo : currentImageInfos)
                {
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = frequencyState.UniformTextures.at(textureIndex)->GetTextureVkImageView();
                    imageInfo.sampler = VK_NULL_HANDLE;
                }
                imageInfos.push_back(std::move(currentImageInfos));

                VkWriteDescriptorSet textureWrite = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frequencyState.DescriptorSets[frameIndex],
                    .dstBinding = static_cast<uint32_t>(textureIndex),
                    .descriptorCount = static_cast<uint32_t>(descriptorCount),
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = imageInfos.back().data(),
                };
                descriptorWrites.push_back(textureWrite);
            }
            for (size_t samplerIndex : frequencyInfo.SamplerIndices)
            {
                size_t descriptorCount = descriptorSetConfig.Bindings[samplerIndex].descriptorCount;
                std::vector<VkDescriptorImageInfo> currentImageInfos(descriptorCount);
                // currentImageInfos.reserve(descriptorCount);
                for (VkDescriptorImageInfo& imageInfo : currentImageInfos)
                {
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageInfo.imageView = VK_NULL_HANDLE;
                    imageInfo.sampler = frequencyState.UniformSamplers.at(samplerIndex)->GetVkSampler();
                }
                imageInfos.push_back(std::move(currentImageInfos));

                VkWriteDescriptorSet samplerWrite = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frequencyState.DescriptorSets[frameIndex],
                    .dstBinding = static_cast<uint32_t>(samplerIndex),
                    .descriptorCount = static_cast<uint32_t>(descriptorCount),
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .pImageInfo = imageInfos.back().data(),
                };
                descriptorWrites.push_back(samplerWrite);
            }
        }

        if (!descriptorWrites.empty())
        {
            vkUpdateDescriptorSets(rendererBackend->GetVkDeviceWrapper().GetLogicalDevice(),
                                   static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        std::vector<VulkanPipeline>& pipelineArray =
            m_ShaderConfig.Flags & ShaderFlagBits::kWireframe ? m_WireframesPipelines : m_Pipelines;
        vkCmdBindDescriptorSets(rendererBackend->GetCurrentGraphicsCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineArray[m_BoundPipelineIndex].Layout, static_cast<uint32_t>(descriptorSetIndex),
                                1, &frequencyState.DescriptorSets[frameIndex], 0, nullptr);
    }

    void VulkanShader::PrepareShaderData()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VulkanDeviceWrapper deviceWrapper = rendererBackend->GetVkDeviceWrapper();

        // NOTE: Descriptor sets and UBO slots are per frame in flight: the in-flight fence guarantees GPU finished
        // with the slot of the current frame
        size_t framesInFlight = rendererBackend->GetVkSwapchain().GetMaxFramesInFlight();

        m_RequiredUboAlignment = deviceWrapper.GetMinUniformBufferOffsetAligment();

        bool isHasPerFrame = !m_ShaderConfig.UniformsPerFrame.empty();
        bool isHasPerGroup = !m_ShaderConfig.UniformsPerGroup.empty();
        bool isHasPerDraw = !m_ShaderConfig.UniformsPerDraw.empty();

        size_t perFrameSamplerCount = GetUniformSamplerCount(m_ShaderConfig.UniformsPerFrame) * framesInFlight;
        size_t perGroupSamplerCount = GetUniformSamplerCount(m_ShaderConfig.UniformsPerGroup) * framesInFlight;
        size_t perDrawSamplerCount = GetUniformSamplerCount(m_ShaderConfig.UniformsPerDraw) * framesInFlight;
        size_t maxSamplerCount = perFrameSamplerCount + perGroupSamplerCount + perDrawSamplerCount;

        size_t perFrameImageCount = GetUniformTextureCount(m_ShaderConfig.UniformsPerFrame) * framesInFlight;
        size_t perGroupImageCount = GetUniformTextureCount(m_ShaderConfig.UniformsPerGroup) * framesInFlight;
        size_t perDrawImageCount = GetUniformTextureCount(m_ShaderConfig.UniformsPerDraw) * framesInFlight;
        size_t maxImageCount = perFrameImageCount + perGroupImageCount + perDrawImageCount;

        // NOTE: All buffer uniforms of the frequency are packed in one UBO binding
        size_t perFrameUboCount = (GetUniformBufferCount(m_ShaderConfig.UniformsPerFrame) > 0 ? 1 : 0) * framesInFlight;
        size_t perGroupUboCount = (GetUniformBufferCount(m_ShaderConfig.UniformsPerGroup) > 0 ? 1 : 0) * framesInFlight;
        // NOTE: Per-draw buffer uniforms are push constants
        size_t perDrawUboCount = 0;
        size_t maxUboCount = perFrameUboCount + perGroupUboCount + perDrawUboCount;

        size_t perFrameDescriptorSetCount = (isHasPerFrame ? 1 : 0) * framesInFlight;
        size_t perGroupDescriptorSetCount = (isHasPerGroup ? 1 : 0) * m_ShaderConfig.MaxGroups * framesInFlight;
        // TODO: may be this zero?
        size_t perDrawDescriptorSetCount = (isHasPerDraw ? 1 : 0) * m_ShaderConfig.MaxDrawIds * framesInFlight;
        m_MaxDescriptorSetCount = perFrameDescriptorSetCount + perGroupDescriptorSetCount + perDrawDescriptorSetCount;

        m_PoolSizes.reserve(3);
        if (maxUboCount > 0)
        {
            m_PoolSizes.emplace_back(VkDescriptorPoolSize {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<uint32_t>(maxUboCount),
            });
        }
        if (maxSamplerCount > 0)
        {
            m_PoolSizes.emplace_back(VkDescriptorPoolSize {
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = static_cast<uint32_t>(maxSamplerCount),
            });
            m_PoolSizes.emplace_back(VkDescriptorPoolSize {
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = static_cast<uint32_t>(maxImageCount),
            });
        }

        // TODO: setup descriptor set configs for per-frame, per-group, per-draw
        if (isHasPerFrame)
        {
            m_DescriptorSetConfigs.emplace_back(
                SetupDescriptorSetConfigAndFrequency(true, m_ShaderConfig.UniformsPerFrame, m_PerFrameInfo));
            m_PerFrameInfo.UboStride = GetAligned(m_PerFrameInfo.UboSize, m_RequiredUboAlignment);
            m_VulkanDescriptorSetConfigIndexPerFrame = m_DescriptorSetConfigs.size() - 1;
        }
        if (isHasPerGroup)
        {
            m_DescriptorSetConfigs.emplace_back(
                SetupDescriptorSetConfigAndFrequency(true, m_ShaderConfig.UniformsPerGroup, m_PerGroupInfo));

            m_PerGroupInfo.UboStride = GetAligned(m_PerGroupInfo.UboSize, m_RequiredUboAlignment);
            m_VulkanDescriptorSetConfigIndexPerGroup = m_DescriptorSetConfigs.size() - 1;
        }
        if (isHasPerDraw)
        {
            m_DescriptorSetConfigs.emplace_back(
                SetupDescriptorSetConfigAndFrequency(false, m_ShaderConfig.UniformsPerDraw, m_PerDrawInfo));
            m_PerDrawInfo.UboStride = GetAligned(m_PerDrawInfo.UboSize, 128);
            m_VulkanDescriptorSetConfigIndexPerDraw = m_DescriptorSetConfigs.size() - 1;
        }

        m_DescriptorSetLayouts.resize(m_DescriptorSetConfigs.size());
    }

    VulkanDescriptorSetConfig
    VulkanShader::SetupDescriptorSetConfigAndFrequency(bool _IsNeedDoUniformBuffers,
                                                       const std::vector<ShaderUniform>& _Uniforms,
                                                       VulkanShaderFrequencyInfo& _OutFrequencyInfo)
    {
        // NOTE: All buffer uniforms are packed into one UBO (std140 layout must match declaration order in GLSL)
        size_t uniformBufferCount = _IsNeedDoUniformBuffers && GetUniformBufferCount(_Uniforms) > 0 ? 1 : 0;
        size_t uniformSamplerCount = GetUniformSamplerCount(_Uniforms);
        size_t uniformTextureCount = GetUniformTextureCount(_Uniforms);

        size_t totalBindingCount = uniformBufferCount + uniformSamplerCount + uniformTextureCount;

        _OutFrequencyInfo.UniformBindings.resize(_Uniforms.size(), 0);
        _OutFrequencyInfo.UniformOffsets.resize(_Uniforms.size(), 0);

        VulkanDescriptorSetConfig result;

        if (totalBindingCount == 0)
        {
            return result;
        }

        result.Bindings.reserve(totalBindingCount);

        // UBO is always at binding 0, textures and samplers follow in declaration order
        if (uniformBufferCount > 0)
        {
            result.Bindings.emplace_back(VkDescriptorSetLayoutBinding {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1u,
                .stageFlags = VK_SHADER_STAGE_ALL,
            });
            _OutFrequencyInfo.UboIndices.push_back(0);
        }

        for (size_t i = 0; i < _Uniforms.size(); ++i)
        {
            const ShaderUniform& uniform = _Uniforms[i];
            if (IsShaderUniformTypeSampler(uniform.Type) || IsShaderUniformTypeTexture(uniform.Type))
            {
                uint32_t bindingIndex = static_cast<uint32_t>(result.Bindings.size());
                result.Bindings.emplace_back(VkDescriptorSetLayoutBinding {
                    .binding = bindingIndex,
                    .descriptorType = IsShaderUniformTypeSampler(uniform.Type) ? VK_DESCRIPTOR_TYPE_SAMPLER
                                                                               : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = glm::max(1u, uniform.ArrayLength),
                    .stageFlags = VK_SHADER_STAGE_ALL,
                });
                _OutFrequencyInfo.UniformBindings[i] = bindingIndex;
                if (IsShaderUniformTypeSampler(uniform.Type))
                {
                    _OutFrequencyInfo.SamplerIndices.push_back(bindingIndex);
                }
                else
                {
                    _OutFrequencyInfo.TextureIndices.push_back(bindingIndex);
                }
            }
            else if (uniformBufferCount > 0)
            {
                _OutFrequencyInfo.UniformBindings[i] = 0;
                _OutFrequencyInfo.UniformOffsets[i] = _OutFrequencyInfo.UboSize;
                _OutFrequencyInfo.UboSize += uniform.Size * glm::max(1u, uniform.ArrayLength);
            }
        }

        return result;
    }

    void VulkanShader::SetupFrequencyState(bool _IsNeedDoUniformBuffers, ShaderUpdateFrequency _Frequency)
    {
        if (GetShaderUniformsForFrequency(_Frequency).empty())
        {
            return;
        }

        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        size_t framesInFlight = rendererBackend->GetVkSwapchain().GetMaxFramesInFlight();

        size_t descriptorSetIndex = GetVulkanDescriptorSetConfigIndexForFrequency(_Frequency);
        const VulkanShaderFrequencyInfo& frequencyInfo = GetVulkanShaderFrequencyInfoForFrequency(_Frequency);
        VulkanShaderFrequencyState& frequencyState = GetVulkanShaderFrequencyStateForFrequency(_Frequency);

        // TODO: fill all state data with default texutres and samplers

        std::vector<VkDescriptorSetLayout> layouts;
        layouts.resize(framesInFlight, m_DescriptorSetLayouts[descriptorSetIndex]);
        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_DescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(framesInFlight),
            .pSetLayouts = layouts.data(),
        };
        frequencyState.DescriptorSets.resize(framesInFlight);
        VkResult allocateResult =
            vkAllocateDescriptorSets(logicalDevice, &allocInfo, frequencyState.DescriptorSets.data());
        if (allocateResult != VK_SUCCESS)
        {
            VEGA_CORE_CRITICAL("Failed to allocate descriptor sets for shader {} with error: {}", m_ShaderConfig.Name,
                               VulkanResultString(allocateResult, true));
            VEGA_CORE_ASSERT(false, "Failed to allocate descriptor sets!");
        }

        for (uint32_t i = 0; i < framesInFlight; ++i)
        {
            VK_SET_DEBUG_OBJECT_NAME(
                rendererBackend->GetVkContext().PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                VK_OBJECT_TYPE_DESCRIPTOR_SET, frequencyState.DescriptorSets[i],
                std::format("desc_set_shader_{}_set_idx_{}_frame_idx_{}", m_ShaderConfig.Name, descriptorSetIndex, i)
                    .c_str());
        }

        if (!_IsNeedDoUniformBuffers || frequencyInfo.UboSize == 0)
        {
            return;
        }

        frequencyState.UboData.assign(frequencyInfo.UboSize, 0);

        // UBO descriptor of each frame set always points to the same slot, so it is written only once
        std::vector<VkDescriptorBufferInfo> bufferInfos(framesInFlight);
        std::vector<VkWriteDescriptorSet> descriptorWrites(framesInFlight);
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            bufferInfos[i] = VkDescriptorBufferInfo {
                .buffer = m_UniformBuffer->GetVkBuffer(),
                .offset = i * m_UniformBufferFrameStride + frequencyInfo.UboOffset,
                .range = frequencyInfo.UboSize,
            };
            descriptorWrites[i] = VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = frequencyState.DescriptorSets[i],
                .dstBinding = static_cast<uint32_t>(frequencyInfo.UboIndices.front()),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfos[i],
            };
        }

        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                               0, nullptr);
    }

    void VulkanShader::CreateUniformBuffer()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        size_t framesInFlight = rendererBackend->GetVkSwapchain().GetMaxFramesInFlight();

        // NOTE: Strides are aligned to minUniformBufferOffsetAlignment, so every block offset is aligned too
        m_PerFrameInfo.UboOffset = 0;
        m_PerGroupInfo.UboOffset = m_PerFrameInfo.UboStride;
        m_UniformBufferFrameStride = m_PerFrameInfo.UboStride + m_PerGroupInfo.UboStride;

        if (m_UniformBufferFrameStride == 0)
        {
            return;
        }

        m_UniformBuffer = CreateRef<VulkanRenderBuffer>(RenderBufferProps {
            .Name = std::format("{}_uniform_buffer", m_ShaderConfig.Name),
            .Type = RenderBufferType::kUniform,
            .ElementSize = m_UniformBufferFrameStride,
            .ElementCount = framesInFlight,
        });

        // NOTE: Memory is host coherent, so it stays mapped for the whole shader lifetime without flushes
        m_MappedUniformBuffer = static_cast<uint8_t*>(m_UniformBuffer->MapMemory());
    }

    void VulkanShader::DestroyUniformBuffer()
    {
        if (!m_UniformBuffer)
        {
            return;
        }

        m_UniformBuffer->UnmapMemory();
        m_MappedUniformBuffer = nullptr;

        m_UniformBuffer->Destroy();
        m_UniformBuffer = nullptr;
        m_UniformBufferFrameStride = 0;
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
                .Winding = RendererWinding::kRendererWindingCounterClockwise,
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

    size_t VulkanShader::GetUniformSamplerCount(const std::vector<ShaderUniform>& _Uniforms) const
    {
        return std::count_if(_Uniforms.begin(), _Uniforms.end(),
                             [](const ShaderUniform& uniform) { return IsShaderUniformTypeSampler(uniform.Type); });
    }

    size_t VulkanShader::GetUniformTextureCount(const std::vector<ShaderUniform>& _Uniforms) const
    {
        return std::count_if(_Uniforms.begin(), _Uniforms.end(),
                             [](const ShaderUniform& uniform) { return IsShaderUniformTypeTexture(uniform.Type); });
    }

    size_t VulkanShader::GetUniformBufferCount(const std::vector<ShaderUniform>& _Uniforms) const
    {
        return std::count_if(_Uniforms.begin(), _Uniforms.end(), [](const ShaderUniform& uniform) {
            return !(IsShaderUniformTypeSampler(uniform.Type) || IsShaderUniformTypeTexture(uniform.Type));
        });
    }

    const std::vector<ShaderUniform>&
    VulkanShader::GetShaderUniformsForFrequency(ShaderUpdateFrequency _Frequency) const
    {
        switch (_Frequency)
        {
            case ShaderUpdateFrequency::kPerFrame: return m_ShaderConfig.UniformsPerFrame;
            case ShaderUpdateFrequency::kPerGroup: return m_ShaderConfig.UniformsPerGroup;
            case ShaderUpdateFrequency::kPerDraw: return m_ShaderConfig.UniformsPerDraw;
            default: VEGA_CORE_ASSERT(false, "Unknown ShaderUpdateFrequency!"); return m_ShaderConfig.UniformsPerFrame;
        }
    }

    const VulkanShaderFrequencyInfo&
    VulkanShader::GetVulkanShaderFrequencyInfoForFrequency(ShaderUpdateFrequency _Frequency) const
    {
        switch (_Frequency)
        {
            case ShaderUpdateFrequency::kPerFrame: return m_PerFrameInfo;
            case ShaderUpdateFrequency::kPerGroup: return m_PerGroupInfo;
            case ShaderUpdateFrequency::kPerDraw: return m_PerDrawInfo;
            default: VEGA_CORE_ASSERT(false, "Unknown ShaderUpdateFrequency!"); return m_PerFrameInfo;
        }
    }

    VulkanShaderFrequencyState&
    VulkanShader::GetVulkanShaderFrequencyStateForFrequency(ShaderUpdateFrequency _Frequency)
    {
        switch (_Frequency)
        {
            case ShaderUpdateFrequency::kPerFrame: return m_PerFrameState;
            case ShaderUpdateFrequency::kPerGroup: return m_PerGroupState;
            case ShaderUpdateFrequency::kPerDraw: return m_PerDrawState;
            default: VEGA_CORE_ASSERT(false, "Unknown ShaderUpdateFrequency!"); return m_PerFrameState;
        }
    }

    const VulkanDescriptorSetConfig&
    VulkanShader::GetVulkanDescriptorSetConfigForFrequency(ShaderUpdateFrequency _Frequency) const
    {
        switch (_Frequency)
        {
            case ShaderUpdateFrequency::kPerFrame:
                return m_DescriptorSetConfigs[m_VulkanDescriptorSetConfigIndexPerFrame];
            case ShaderUpdateFrequency::kPerGroup:
                return m_DescriptorSetConfigs[m_VulkanDescriptorSetConfigIndexPerGroup];
            case ShaderUpdateFrequency::kPerDraw:
                return m_DescriptorSetConfigs[m_VulkanDescriptorSetConfigIndexPerDraw];
            default: VEGA_CORE_ASSERT(false, "Unknown ShaderUpdateFrequency!"); return m_DescriptorSetConfigs[0];
        }
    }

    size_t VulkanShader::GetVulkanDescriptorSetConfigIndexForFrequency(ShaderUpdateFrequency _Frequency) const
    {
        switch (_Frequency)
        {
            case ShaderUpdateFrequency::kPerFrame: return m_VulkanDescriptorSetConfigIndexPerFrame;
            case ShaderUpdateFrequency::kPerGroup: return m_VulkanDescriptorSetConfigIndexPerGroup;
            case ShaderUpdateFrequency::kPerDraw: return m_VulkanDescriptorSetConfigIndexPerDraw;
            default: VEGA_CORE_ASSERT(false, "Unknown ShaderUpdateFrequency!"); return 0;
        }
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
