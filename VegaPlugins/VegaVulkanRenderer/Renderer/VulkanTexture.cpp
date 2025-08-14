#include "VulkanTexture.hpp"

#include "Vega/Core/Assert.hpp"

#include "Utils/VulkanUtils.hpp"
#include "VulkanBase.hpp"
#include "VulkanRendererBackend.hpp"

namespace Vega
{

    static VkFormat ChannelsCountToVkFormat(uint32_t _ChannelCount, VkFormat _DefaultFormat);

    static VkImageType TextureTypeToVkImageType(TextureProps::TextureType _Type);

    static VkImageViewType TextureTypeToVkImageViewType(TextureProps::TextureType _Type);

    void VulkanTexture::VulkanCreate(std::string_view _Name, const TextureProps& _Props, VkFormat _Format,
                                     VkImageTiling _Tiling, VkImageUsageFlags _Usage,
                                     VkMemoryPropertyFlags _MemoryFlags, VkImageAspectFlags _ViewAspectFlags)
    {
        m_Props = _Props;

        m_ImageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = TextureTypeToVkImageType(_Props.Type),
            .format = _Format,
            .extent = { .width = _Props.Width, .height = _Props.Height, .depth = 1 },
            .mipLevels = _Props.MipLevels,
            .arrayLayers = _Props.ArraySize,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = _Tiling,
            .usage = _Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        if (_Props.Type == TextureProps::TextureType::kCubeMap)
        {
            m_ImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        m_ImageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = nullptr,
            .viewType = TextureTypeToVkImageViewType(_Props.Type),
            .format = _Format,
            .subresourceRange = {
                .aspectMask = _ViewAspectFlags,
                .baseMipLevel = 0,
                .levelCount = _Props.MipLevels,
                .baseArrayLayer = 0,
                .layerCount = _Props.ArraySize,
            },
        };

        if (_Props.ArraySize > 1)
        {
            m_ImageArrayViewsInfos.resize(_Props.ArraySize);
            m_ImageArrayViews.resize(_Props.ArraySize);
            TextureProps::TextureType viewType = TextureProps::TextureType::k2D;

            for (uint32_t i = 0; i < _Props.ArraySize; i++)
            {
                m_ImageArrayViewsInfos[i] = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = nullptr,
                    .viewType = TextureTypeToVkImageViewType(viewType),
                    .format = _Format,
                    .subresourceRange = {
                        .aspectMask = _ViewAspectFlags,
                        .baseMipLevel = 0,
                        .levelCount = _Props.MipLevels,
                        .baseArrayLayer = i,
                        .layerCount = 1,
                    },
                };
            }
        }

        VulkanCreate(_Name);
    }

    void VulkanTexture::VulkanCreate(std::string_view _Name)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        const VulkanContext& context = rendererBackend->GetVkContext();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();

        VK_CHECK(vkCreateImage(logicalDevice, &m_ImageInfo, context.VkAllocator, &m_Image));

        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_IMAGE, m_Image,
                                 _Name.data());

        vkGetImageMemoryRequirements(logicalDevice, m_Image, &m_MemoryRequirements);

        uint32_t memoryTypeIndex = rendererBackend->GetVkDeviceWrapper().GetMemoryTypeIndex(
            m_MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo memoryAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = m_MemoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };
        VkResult allocateResult =
            vkAllocateMemory(logicalDevice, &memoryAllocateInfo, context.VkAllocator, &m_ImageMemory);
        if (!VulkanResultIsSuccess(allocateResult))
        {
            VEGA_CORE_ERROR("Failed to allocate memory for image: {}", VulkanResultString(allocateResult, true));
            VEGA_CORE_ASSERT(false, "Failed to allocate memory for image");
            return;
        }

        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_DEVICE_MEMORY,
                                 m_ImageMemory, _Name.data());

        VK_CHECK(vkBindImageMemory(logicalDevice, m_Image, m_ImageMemory, 0));

        m_ImageViewInfo.image = m_Image;
        VK_CHECK(vkCreateImageView(logicalDevice, &m_ImageViewInfo, context.VkAllocator, &m_ImageView));
        VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice, VK_OBJECT_TYPE_IMAGE_VIEW,
                                 m_ImageView, std::format("{}_view", _Name.data()).c_str());

        if (m_Props.ArraySize > 1)
        {
            for (uint32_t i = 0; i < m_Props.ArraySize; i++)
            {
                m_ImageArrayViewsInfos[i].image = m_Image;
                VK_CHECK(vkCreateImageView(logicalDevice, &m_ImageArrayViewsInfos[i], context.VkAllocator,
                                           &m_ImageArrayViews[i]));
                VK_SET_DEBUG_OBJECT_NAME(context.PfnSetDebugUtilsObjectNameEXT, logicalDevice,
                                         VK_OBJECT_TYPE_IMAGE_VIEW, m_ImageArrayViews[i],
                                         std::format("{}_view_layer_{}", _Name.data(), i).c_str());
            }
        }
    }

    void VulkanTexture::Create(std::string_view _Name, const TextureProps& _Props)
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VkImageUsageFlags usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageAspectFlags aspect;
        VkFormat imageFormat;
        if (_Props.Flags & TextureProps::kDepthAttachment)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageFormat = rendererBackend->GetVkDeviceWrapper().GetDepthFormat();
        }
        else
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            imageFormat = ChannelsCountToVkFormat(_Props.ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);
        }

        VulkanCreate(_Name, _Props, imageFormat, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     aspect);
    }

    void VulkanTexture::Create(std::string_view _Name, const TextureProps& _Props, uint8_t* _Data)
    {
        Create(_Name, _Props);

        // TODO: Implement
    }

    void VulkanTexture::CreateSwapchainTexture(VkImage _Image, VkSurfaceFormatKHR _ImageFormat,
                                               const TextureProps& _Props)
    {
        m_Props = _Props;
        // _ImageFormat.format;

        m_Image = _Image;

        m_ImageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_Image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = _ImageFormat.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VK_CHECK(vkCreateImageView(rendererBackend->GetVkDeviceWrapper().GetLogicalDevice(), &m_ImageViewInfo,
                                   rendererBackend->GetVkContext().VkAllocator, &m_ImageView));
    }

    void VulkanTexture::Destroy()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        if (m_ImageView)
        {
            vkDestroyImageView(logicalDevice, m_ImageView, vkAllocator);

            for (VkImageView imageView : m_ImageArrayViews)
            {
                vkDestroyImageView(logicalDevice, imageView, vkAllocator);
            }

            m_ImageView = nullptr;
            m_ImageArrayViews.clear();
            m_ImageArrayViewsInfos.clear();
        }

        vkFreeMemory(logicalDevice, m_ImageMemory, vkAllocator);
        m_ImageMemory = nullptr;

        vkDestroyImage(logicalDevice, m_Image, vkAllocator);
        m_Image = nullptr;
    }

    void VulkanTexture::Resize(std::string_view _Name, uint32_t _NewWidth, uint32_t _NewHeight)
    {
        m_Props.Width = _NewWidth;
        m_Props.Height = _NewHeight;

        m_ImageInfo.extent.width = _NewWidth;
        m_ImageInfo.extent.height = _NewHeight;

        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkDevice logicalDevice = rendererBackend->GetVkDeviceWrapper().GetLogicalDevice();
        const VkAllocationCallbacks* vkAllocator = rendererBackend->GetVkContext().VkAllocator;

        vkDestroyImage(logicalDevice, m_Image, vkAllocator);
        vkFreeMemory(logicalDevice, m_ImageMemory, vkAllocator);

        if (m_ImageView)
        {
            vkDestroyImageView(logicalDevice, m_ImageView, vkAllocator);

            for (VkImageView imageView : m_ImageArrayViews)
            {
                vkDestroyImageView(logicalDevice, imageView, vkAllocator);
            }
        }

        VulkanCreate(_Name);
    }

    static VkFormat ChannelsCountToVkFormat(uint32_t _ChannelCount, VkFormat _DefaultFormat)
    {
        switch (_ChannelCount)
        {
            case 1: return VK_FORMAT_R8_UNORM;
            case 2: return VK_FORMAT_R8G8_UNORM;
            case 3: return VK_FORMAT_R8G8B8_UNORM;
            case 4: return VK_FORMAT_R8G8B8A8_UNORM;
        }

        VEGA_CORE_ASSERT(false, "ChannelsCountToVkFormat: Unknown ChannelCount!")

        return _DefaultFormat;
    }

    static VkImageType TextureTypeToVkImageType(TextureProps::TextureType _Type)
    {
        switch (_Type)
        {
            case TextureProps::TextureType::k2D: return VK_IMAGE_TYPE_2D;
            case TextureProps::TextureType::kCubeMap: return VK_IMAGE_TYPE_2D;
            case TextureProps::TextureType::k3D: return VK_IMAGE_TYPE_3D;
            case TextureProps::TextureType::kArray: return VK_IMAGE_TYPE_2D;
        }

        VEGA_CORE_ASSERT(false, "TextureTypeToVkImageType: Unknown TextureType!")
        return VK_IMAGE_TYPE_2D;
    }

    static VkImageViewType TextureTypeToVkImageViewType(TextureProps::TextureType _Type)
    {
        switch (_Type)
        {
            case TextureProps::TextureType::k2D: return VK_IMAGE_VIEW_TYPE_2D;
            case TextureProps::TextureType::kCubeMap: return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureProps::TextureType::k3D: return VK_IMAGE_VIEW_TYPE_3D;
            case TextureProps::TextureType::kArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }

        VEGA_CORE_ASSERT(false, "TextureTypeToVkImageViewType: Unknown TextureType!")
        return VK_IMAGE_VIEW_TYPE_2D;
    }

}    // namespace Vega
