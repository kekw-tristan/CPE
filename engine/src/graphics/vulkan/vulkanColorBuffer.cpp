#include "vulkanColorBuffer.h"

#include "vulkanImage.h"
#include "vulkanDevice.h"
#include "vulkanSwapchain.h"
#include "vulkanCommands.h"

#include <iostream>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanColorBuffer::cVulkanColorBuffer()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanColorBuffer::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapchain, cVulkanCommands& _rCommands)
    {
        VkExtent2D extent = _rSwapchain.GetExtent();

        m_image.Create(
            _rDevice,
            extent.width,
            extent.height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            _rDevice.GetMSAASamples()
        );

        m_resolveImage.Create(
            _rDevice,
            extent.width,
            extent.height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_SAMPLE_COUNT_1_BIT
        );

        VkCommandBuffer commandBuffer = _rCommands.BeginSingleTimeCommands(_rDevice);

        m_image.TransitionLayout(
            _rDevice,
            commandBuffer,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        m_resolveImage.TransitionLayout(
            _rDevice,
            commandBuffer,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        
        _rCommands.EndSingleTimeCommands(_rDevice, commandBuffer);
    }   

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanColorBuffer::ShutDown(cVulkanDevice& _rDevice)
    {
        m_image.Destroy(_rDevice);
        m_resolveImage.Destroy(_rDevice);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanColorBuffer::GetImageView() const
    {
        return m_image.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanColorBuffer::GetResolveImageView() const
    {
        return m_resolveImage.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImage cVulkanColorBuffer::GetResolveImage() const
    {
        return m_resolveImage.GetImage();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkFormat cVulkanColorBuffer::GetFormat() const
    {
        return m_image.GetFormat();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
