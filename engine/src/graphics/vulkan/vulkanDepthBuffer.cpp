#include "vulkanDepthBuffer.h"

#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanCommands.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{
    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanDepthBuffer::cVulkanDepthBuffer()
        : m_pImage(VK_NULL_HANDLE)
        , m_pMemory(VK_NULL_HANDLE)
        , m_pImageView(VK_NULL_HANDLE)
        , m_format(VK_FORMAT_D32_SFLOAT)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDepthBuffer::Init(cVulkanDevice &_rDevice, cVulkanSwapchain &_rSwapchain, cVulkanCommands &_rCommands)
    {
        VkExtent2D extent = _rSwapchain.GetExtent();

        CreateImage(_rDevice, extent.width, extent.height);
        CreateImageView(_rDevice);
        TransitionImageLayout(_rDevice, _rCommands);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDepthBuffer::ShutDown(cVulkanDevice &_rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_pImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, m_pImageView, nullptr);
            m_pImageView = VK_NULL_HANDLE;
        }

        if (m_pImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_pImage, nullptr);
            m_pImage = VK_NULL_HANDLE;
        }

        if (m_pMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_pMemory, nullptr);
            m_pMemory = VK_NULL_HANDLE;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanDepthBuffer::GetImageView() const
    {
        return m_pImageView;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkFormat cVulkanDepthBuffer::GetFormat() const
    {
        return m_format;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDepthBuffer::CreateImage(cVulkanDevice &_rDevice, uint32_t _width, uint32_t _height)
    {
        VkImageCreateInfo imageInfo{};

        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = _width;
        imageInfo.extent.height = _height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = m_format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(_rDevice.GetDevice(), &imageInfo, nullptr, &m_pImage) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan depth image!");
        }
        
        VkMemoryRequirements memoryRequirements;

        vkGetImageMemoryRequirements(_rDevice.GetDevice(), m_pImage, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo{};

        allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext             = nullptr;
        allocInfo.allocationSize    = memoryRequirements.size;
        allocInfo.memoryTypeIndex   = _rDevice.FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(_rDevice.GetDevice(), &allocInfo, nullptr, &m_pMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate Vulkan depth image memory"); 
        }

        if (vkBindImageMemory(_rDevice.GetDevice(), m_pImage, m_pMemory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind Vulkan depth image memory!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDepthBuffer::CreateImageView(cVulkanDevice &_rDevice)
    {
        VkImageViewCreateInfo viewInfo{};

        viewInfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image      = m_pImage;
        viewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format     = m_format;

        viewInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel    = 0;
        viewInfo.subresourceRange.levelCount      = 1;
        viewInfo.subresourceRange.baseArrayLayer  = 0;
        viewInfo.subresourceRange.layerCount      = 1;

        if (vkCreateImageView(_rDevice.GetDevice(), &viewInfo, nullptr, &m_pImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan depth image view!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDepthBuffer::TransitionImageLayout(cVulkanDevice &_rDevice, cVulkanCommands &_rCommands)
    {
        VkCommandBuffer pCommandBuffer = _rCommands.BeginSingleTimeCommands(_rDevice);


        VkImageMemoryBarrier barrier{};

        barrier.sType       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = m_pImage;

        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(pCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        _rCommands.EndSingleTimeCommands(_rDevice, pCommandBuffer);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------