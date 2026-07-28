#include "vulkanImage.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <stdexcept>

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
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------


    cVulkanImage::~cVulkanImage()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanImage::Create(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height, VkFormat _format, VkImageUsageFlags _usage, VkImageAspectFlags _aspect, VkSampleCountFlagBits _samples)
    {
        m_format = _format;

        VkImageCreateInfo imageInfo{};

        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = _width;
        imageInfo.extent.height = _height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = _format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = _usage;
        imageInfo.samples       = _samples;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

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
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = _format;
        viewInfo.subresourceRange.aspectMask     = _aspect;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

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
        VkImageMemoryBarrier barrier{};

        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = _oldLayout;
        barrier.newLayout           = _newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = m_image;

        barrier.subresourceRange.aspectMask     = _aspect;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask   = 0;
            barrier.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage                = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask   = 0;
            barrier.dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage                = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage                = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }

        vkCmdPipelineBarrier(_pCommands, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

}

// -------------------------------------------------------------------------------------------------------------------------