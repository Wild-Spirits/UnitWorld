#pragma once

#include "Vega/Renderer/Texture.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <vulkan/vulkan_core.h>

namespace Vega
{

    struct VulkanTextureTransitionMaskResult
    {
        VkPipelineStageFlags2 SrcStageMask;
        VkAccessFlags2 SrcAccessMask;
        VkPipelineStageFlags2 DstStageMask;
        VkAccessFlags2 DstAccessMask;
    };

    class VulkanTexture : public Texture
    {
    public:
        void VulkanCreate(std::string_view _Name, const TextureProps& _Props, VkFormat _Format, VkImageTiling _Tiling,
                          VkImageUsageFlags _Usage, VkMemoryPropertyFlags _MemoryFlags, VkImageAspectFlags _Aspect);

        void VulkanCreate(std::string_view _Name);

        VulkanTextureTransitionMaskResult GetTransitionMask(VkImageLayout _OldLayout, VkImageLayout _NewLayout);
        void TransitionImageLayout(VkImageLayout _OldLayout, VkImageLayout _NewLayout, VkCommandBuffer _CommandBuffer);

        void Create(std::string_view _Name, const TextureProps& _Props) override;
        void Create(std::string_view _Name, const TextureProps& _Props, uint8_t* _Data) override;
        void CreateSwapchainTexture(VkImage _Image, VkSurfaceFormatKHR _Format, const TextureProps& _Props);
        void Destroy() override;
        void Resize(std::string_view _Name, uint32_t _NewWidth, uint32_t _NewHeight) override;

        void* GetTextureGuiId() const override { return reinterpret_cast<void*>(m_DescriptorSet); }
        uint32_t GetWidth() const override { return m_Props.Width; }
        uint32_t GetHeight() const override { return m_Props.Height; }
        uint32_t GetMipLevels() const override { return m_Props.MipLevels; }
        uint32_t GetArraySize() const override { return m_Props.ArraySize; }

        VkImage GetTextureVkImage() const { return m_Image; }
        VkImageView GetTextureVkImageView() const { return m_ImageView; }

        const VkImageViewCreateInfo& GetImageViewCreateInfo() const { return m_ImageViewInfo; }

    protected:
        TextureProps m_Props;

        VkImageCreateInfo m_ImageInfo;
        VkImage m_Image;

        VkImageViewCreateInfo m_ImageViewInfo;
        VkImageView m_ImageView = nullptr;

        VkMemoryRequirements m_MemoryRequirements;
        VkDeviceMemory m_ImageMemory;

        std::vector<VkImageViewCreateInfo> m_ImageArrayViewsInfos;
        std::vector<VkImageView> m_ImageArrayViews;

        VkSampler m_Sampler = VK_NULL_HANDLE;
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
    };

}    // namespace Vega
