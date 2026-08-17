#include "vulkanImage.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanImage::cVulkanImage()
    : m_image    ()
    , m_memory   ()
    , m_imageView()
    , m_format   ()
    , m_width    (0)
    , m_height   (0)
    , m_arrayLayers(1)
    , m_mipLevels(1)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------


    cVulkanImage::~cVulkanImage()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::Create(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height, VkFormat _format, VkImageUsageFlags _usage, VkImageAspectFlags _aspect, VkSampleCountFlagBits _samples, uint32_t _arrayLayers, VkImageViewType _viewType, VkImageCreateFlags _flags, uint32_t _mipLevels)
    {
        m_format        = _format;
        m_width         = _width; 
        m_height        = _height; 
        m_arrayLayers   = _arrayLayers;
        m_mipLevels     = _mipLevels;

        VkImageCreateInfo imageInfo{};

        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = _width;
        imageInfo.extent.height = _height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = m_mipLevels;
        imageInfo.arrayLayers   = m_arrayLayers;
        imageInfo.format        = _format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = _usage;
        imageInfo.samples       = _samples;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags         = _flags;

        if (vkCreateImage(_rDevice.GetDevice(), &imageInfo, nullptr, &m_image) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan image!");
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(_rDevice.GetDevice(), m_image, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo{};

        allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize    = memoryRequirements.size;
        allocInfo.memoryTypeIndex   = _rDevice.FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(_rDevice.GetDevice(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate Vulkan image memory!");
        }

        if (vkBindImageMemory(_rDevice.GetDevice(), m_image, m_memory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind Vulkan image memory!");
        }

        VkImageViewCreateInfo viewInfo{};

        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = m_image;
        viewInfo.viewType                        = _viewType;
        viewInfo.format                          = _format;
        viewInfo.subresourceRange.aspectMask     = _aspect;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = m_mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = _arrayLayers;

        if (vkCreateImageView(_rDevice.GetDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan image view!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::Destroy(cVulkanDevice &_rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }

        if (m_image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::TransitionLayout(cVulkanDevice& _rDevice, VkCommandBuffer _pCommands, VkImageLayout _oldLayout, VkImageLayout _newLayout, VkImageAspectFlags _aspect)
    {
        if (_oldLayout == _newLayout)
        {
            return;
        }

        VkImageMemoryBarrier barrier{};

        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = _oldLayout;
        barrier.newLayout           = _newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = m_image;

        barrier.subresourceRange.aspectMask     = _aspect;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = m_mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = m_arrayLayers;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }
        else if (_oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && _newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (_oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && _newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }
        else if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::runtime_error("Unsupported Vulkan image layout transition!");
        }


        vkCmdPipelineBarrier(_pCommands, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::CopyFromBuffer(VkCommandBuffer _pCommands, VkBuffer _sourceBuffer, VkDeviceSize _layerSize, VkImageAspectFlags _aspect)
    {
        std::vector<VkBufferImageCopy> copyRegions(m_arrayLayers);

        for (uint32_t layer = 0; layer < m_arrayLayers; ++layer)
        {
            VkBufferImageCopy& copyRegion = copyRegions[layer];

            copyRegion.bufferOffset = _layerSize * layer;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = _aspect;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = layer;
            copyRegion.imageSubresource.layerCount = 1;

            copyRegion.imageOffset = { 0, 0, 0 };
            copyRegion.imageExtent = { m_width, m_height, 1 };
        }

        vkCmdCopyBufferToImage(_pCommands, _sourceBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::GenerateMipmaps(cVulkanDevice& _rDevice, VkCommandBuffer _pCommands)
    {
        VkFormatProperties formatProperties{};
        vkGetPhysicalDeviceFormatProperties(_rDevice.GetPhysicalDevice(), m_format, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("Vulkan image format does not support linear blitting!");
        }

        int32_t mipWidth    = static_cast<int32_t>(m_width);
        int32_t mipHeight   = static_cast<int32_t>(m_height);

        for (uint32_t mipLevel = 1; mipLevel < m_mipLevels; ++mipLevel)
        {
            VkImageMemoryBarrier barrier{};

            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image                           = m_image;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = mipLevel - 1;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = m_arrayLayers;

            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(_pCommands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            const int32_t nextWidth  = mipWidth  > 1 ? mipWidth  / 2 : 1;
            const int32_t nextHeight = mipHeight > 1 ? mipHeight / 2 : 1;

            VkImageBlit blit{};

            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };

            blit.srcSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel        = mipLevel - 1;
            blit.srcSubresource.baseArrayLayer  = 0;
            blit.srcSubresource.layerCount      = m_arrayLayers;

            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { nextWidth, nextHeight, 1 };

            blit.dstSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel        = mipLevel;
            blit.dstSubresource.baseArrayLayer  = 0;
            blit.dstSubresource.layerCount      = m_arrayLayers;

            vkCmdBlitImage(_pCommands, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask   = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(_pCommands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            mipWidth  = nextWidth;
            mipHeight = nextHeight;
        }

        VkImageMemoryBarrier barrier{};

        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image                           = m_image;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = m_mipLevels - 1;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = m_arrayLayers;

        barrier.oldLayout       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(_pCommands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::CopyMipChainFromBuffer(VkCommandBuffer _pCommands, VkBuffer _sourceBuffer, VkDeviceSize _bytesPerPixel, VkImageAspectFlags _aspect)
    {
        std::vector<VkBufferImageCopy> copyRegions;
        copyRegions.reserve(static_cast<size_t>(m_mipLevels) * m_arrayLayers);

        VkDeviceSize bufferOffset = 0;

        uint32_t mipWidth  = m_width;
        uint32_t mipHeight = m_height;

        for (uint32_t mipLevel = 0; mipLevel < m_mipLevels; ++mipLevel)
        {
            const VkDeviceSize layerSize = static_cast<VkDeviceSize>(mipWidth) * mipHeight * _bytesPerPixel;

            for (uint32_t layer = 0; layer < m_arrayLayers; ++layer)
            {
                VkBufferImageCopy copyRegion{};

                copyRegion.bufferOffset      = bufferOffset + layerSize * layer;
                copyRegion.bufferRowLength   = 0;
                copyRegion.bufferImageHeight = 0;

                copyRegion.imageSubresource.aspectMask      = _aspect;
                copyRegion.imageSubresource.mipLevel        = mipLevel;
                copyRegion.imageSubresource.baseArrayLayer  = layer;
                copyRegion.imageSubresource.layerCount      = 1;

                copyRegion.imageOffset = { 0, 0, 0 };
                copyRegion.imageExtent = { mipWidth, mipHeight, 1 };

                copyRegions.push_back(copyRegion);
            }

            bufferOffset += layerSize * m_arrayLayers;

            mipWidth  = std::max(1u, mipWidth  / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        vkCmdCopyBufferToImage(_pCommands, _sourceBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanImage::GetImageView() const
    {
        return m_imageView;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImage cVulkanImage::GetImage() const
    {
        return m_image;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkFormat cVulkanImage::GetFormat() const
    {
        return m_format;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanImage::GetWidth() const
    {
        return m_width;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanImage::GetMipLevels() const
    {
        return m_mipLevels;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanImage::GetHeight() const
    {
        return m_height;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------