#pragma once

#include "Vega/Renderer/Texture.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace Vega
{

    class VulkanTexture : public Texture
    {
    public:
        void VulkanCreate(std::string_view _Name, const TextureProps& _Props, VkFormat _Format, VkImageTiling _Tiling,
                          VkImageUsageFlags _Usage, VkMemoryPropertyFlags _MemoryFlags, VkImageAspectFlags _Aspect);

        void VulkanCreate(std::string_view _Name);

        void Create(std::string_view _Name, const TextureProps& _Props) override;
        void Create(std::string_view _Name, const TextureProps& _Props, uint8_t* _Data) override;
        void CreateSwapchainTexture(VkImage _Image, VkSurfaceFormatKHR _Format, const TextureProps& _Props);
        void Destroy() override;
        void Resize(std::string_view _Name, uint32_t _NewWidth, uint32_t _NewHeight) override;

        void* GetTextureId() const override { return (void*)m_Image; }
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
    };

}    // namespace Vega
