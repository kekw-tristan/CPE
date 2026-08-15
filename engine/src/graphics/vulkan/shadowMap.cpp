#include "shadowMap.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    cShadowMap::cShadowMap()
        : m_depthImage()
        , m_sampler(VK_NULL_HANDLE)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cShadowMap::~cShadowMap()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cShadowMap::Create(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height, uint32_t _layerCount)
    {
        m_layerCount = _layerCount;

        m_depthImage.Create(
            _rDevice,
            _width,
            _height,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_SAMPLE_COUNT_1_BIT,
            _layerCount,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY
        );

        m_layerImageViews.resize(_layerCount);

        for (uint32_t layer = 0; layer < _layerCount; ++layer)
        {
            VkImageViewCreateInfo viewInfo{};

            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_depthImage.GetImage();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_depthImage.GetFormat();
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = layer;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(_rDevice.GetDevice(), &viewInfo, nullptr, &m_layerImageViews[layer]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create shadow map layer image view!");
            }
        }

        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(_rDevice.GetDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shadow map sampler!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cShadowMap::Destroy(cVulkanDevice& _rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }

        for (VkImageView imageView : m_layerImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }

        m_layerImageViews.clear();

        m_depthImage.Destroy(_rDevice);

        m_layerCount = 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cShadowMap::GetImageView() const
    {
        return m_depthImage.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImage cShadowMap::GetImage() const
    {
        return m_depthImage.GetImage();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkSampler cShadowMap::GetSampler() const
    {
        return m_sampler;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkFormat cShadowMap::GetFormat() const
    {
        return m_depthImage.GetFormat();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cShadowMap::GetWidth() const
    {
        return m_depthImage.GetWidth();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cShadowMap::GetHeight() const
    {
        return m_depthImage.GetHeight();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cShadowMap::GetLayerImageView(uint32_t _layer) const
    {
        return m_layerImageViews[_layer];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cShadowMap::GetLayerCount() const
    {
        return m_layerCount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanImage& cShadowMap::GetImageResource()
    {
        return m_depthImage;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const cVulkanImage& cShadowMap::GetImageResource() const
    {
        return m_depthImage;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------