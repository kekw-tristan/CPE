#include "vulkanReflectionProbe.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanReflectionProbe::cVulkanReflectionProbe()
        : m_captureImage()
        , m_prefilteredImage()
        , m_captureFaceImageViews{}
        , m_prefilteredFaceMipImageViews()
        , m_sampler(VK_NULL_HANDLE)
        , m_resolution(0)
        , m_mipLevels(1)
    {
        m_captureFaceImageViews.fill(VK_NULL_HANDLE);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanReflectionProbe::~cVulkanReflectionProbe()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanReflectionProbe::Create(cVulkanDevice& _rDevice, uint32_t _resolution)
    {
        if (_resolution == 0)
        {
            throw std::runtime_error("Reflection probe resolution must be greater than zero!");
        }

        m_resolution = _resolution;
        m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(_resolution)))) + 1;

        VkFormatProperties formatProperties{};

        vkGetPhysicalDeviceFormatProperties(_rDevice.GetPhysicalDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, &formatProperties);

        if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0)
        {
            throw std::runtime_error("Reflection probe format does not support linear filtering!");
        }

        if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) == 0 || (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) == 0)
        {
            throw std::runtime_error("Reflection probe format does not support image blitting!");
        }

        const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;

        // ---------------------------------------------------------------------------------------------------------------------
        // Raw capture cubemap
        // ---------------------------------------------------------------------------------------------------------------------

        m_captureImage.Create(
            _rDevice,
            m_resolution,
            m_resolution,
            format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_SAMPLE_COUNT_1_BIT,
            6,
            VK_IMAGE_VIEW_TYPE_CUBE,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            m_mipLevels
        );

        // ---------------------------------------------------------------------------------------------------------------------
        // Prefiltered cubemap
        // ---------------------------------------------------------------------------------------------------------------------

        m_prefilteredImage.Create(
            _rDevice,
            m_resolution,
            m_resolution,
            format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_SAMPLE_COUNT_1_BIT,
            6,
            VK_IMAGE_VIEW_TYPE_CUBE,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            m_mipLevels
        );

        // ---------------------------------------------------------------------------------------------------------------------
        // Capture face views
        // ---------------------------------------------------------------------------------------------------------------------

        for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            VkImageViewCreateInfo viewInfo{};

            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_captureImage.GetImage();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;

            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = faceIndex;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(_rDevice.GetDevice(), &viewInfo, nullptr, &m_captureFaceImageViews[faceIndex]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create reflection probe capture face image view!");
            }
        }

        // ---------------------------------------------------------------------------------------------------------------------
        // Prefiltered face/mip views
        // ---------------------------------------------------------------------------------------------------------------------

        m_prefilteredFaceMipImageViews.resize(static_cast<size_t>(m_mipLevels) * 6, VK_NULL_HANDLE);

        for (uint32_t mipLevel = 0; mipLevel < m_mipLevels; ++mipLevel)
        {
            for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                VkImageViewCreateInfo viewInfo{};

                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = m_prefilteredImage.GetImage();
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = format;

                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = mipLevel;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = faceIndex;
                viewInfo.subresourceRange.layerCount = 1;

                const uint32_t viewIndex = GetPrefilteredViewIndex(faceIndex, mipLevel);

                if (vkCreateImageView(_rDevice.GetDevice(), &viewInfo, nullptr, &m_prefilteredFaceMipImageViews[viewIndex]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create reflection probe prefiltered face/mip image view!");
                }
            }
        }

        // ---------------------------------------------------------------------------------------------------------------------
        // Sampler
        // ---------------------------------------------------------------------------------------------------------------------

        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = VK_FILTER_LINEAR;
        samplerInfo.minFilter               = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.anisotropyEnable        = VK_FALSE;
        samplerInfo.maxAnisotropy           = 1.0f;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod                  = 0.0f;
        samplerInfo.maxLod                  = VK_LOD_CLAMP_NONE;
        samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(_rDevice.GetDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create reflection probe sampler!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanReflectionProbe::Destroy(cVulkanDevice& _rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }

        for (VkImageView& rImageView : m_captureFaceImageViews)
        {
            if (rImageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, rImageView, nullptr);
                rImageView = VK_NULL_HANDLE;
            }
        }

        for (VkImageView& rImageView : m_prefilteredFaceMipImageViews)
        {
            if (rImageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, rImageView, nullptr);
                rImageView = VK_NULL_HANDLE;
            }
        }

        m_prefilteredFaceMipImageViews.clear();

        m_captureImage.Destroy(_rDevice);
        m_prefilteredImage.Destroy(_rDevice);

        m_resolution = 0;
        m_mipLevels = 1;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanImage& cVulkanReflectionProbe::GetCaptureImage()
    {
        return m_captureImage;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanReflectionProbe::GetCaptureImageView() const
    {
        return m_captureImage.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanReflectionProbe::GetCaptureFaceImageView(uint32_t _faceIndex) const
    {
        if (_faceIndex >= 6)
        {
            throw std::runtime_error("Invalid reflection probe face index!");
        }

        return m_captureFaceImageViews[_faceIndex];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanImage& cVulkanReflectionProbe::GetPrefilteredImage()
    {
        return m_prefilteredImage;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanReflectionProbe::GetPrefilteredImageView() const
    {
        return m_prefilteredImage.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanReflectionProbe::GetPrefilteredFaceMipImageView(uint32_t _faceIndex, uint32_t _mipLevel) const
    {
        if (_faceIndex >= 6)
        {
            throw std::runtime_error("Invalid reflection probe face index!");
        }

        if (_mipLevel >= m_mipLevels)
        {
            throw std::runtime_error("Invalid reflection probe mip level!");
        }

        return m_prefilteredFaceMipImageViews[GetPrefilteredViewIndex(_faceIndex, _mipLevel)];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkSampler cVulkanReflectionProbe::GetSampler() const
    {
        return m_sampler;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanReflectionProbe::GetResolution() const
    {
        return m_resolution;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanReflectionProbe::GetMipLevels() const
    {
        return m_mipLevels;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanReflectionProbe::GetMipResolution(uint32_t _mipLevel) const
    {
        if (_mipLevel >= m_mipLevels)
        {
            throw std::runtime_error("Invalid reflection probe mip level!");
        }

        return std::max(1u, m_resolution >> _mipLevel);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanReflectionProbe::GetPrefilteredViewIndex(uint32_t _faceIndex, uint32_t _mipLevel) const
    {
        return _mipLevel * 6 + _faceIndex;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------