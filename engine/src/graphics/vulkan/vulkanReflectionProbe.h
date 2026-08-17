#pragma once

#include "graphics/vulkan/vulkanImage.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

namespace Engine::GFX
{
    class cVulkanDevice;

    class cVulkanReflectionProbe
    {
    public:

        cVulkanReflectionProbe();
        ~cVulkanReflectionProbe();

        cVulkanReflectionProbe(const cVulkanReflectionProbe&) = delete;
        cVulkanReflectionProbe& operator=(const cVulkanReflectionProbe&) = delete;

    public:

        void Create(cVulkanDevice& _rDevice, uint32_t _resolution);
        void Destroy(cVulkanDevice& _rDevice);

    public:


        cVulkanImage& GetCaptureImage();

        VkImageView GetCaptureImageView() const;
        VkImageView GetCaptureFaceImageView(uint32_t _faceIndex) const;

    public:


        cVulkanImage& GetPrefilteredImage();

        VkImageView GetPrefilteredImageView() const;
        VkImageView GetPrefilteredFaceMipImageView(uint32_t _faceIndex, uint32_t _mipLevel) const;

    public:

        VkSampler GetSampler() const;

        uint32_t GetResolution() const;
        uint32_t GetMipLevels() const;
        uint32_t GetMipResolution(uint32_t _mipLevel) const;

    private:

        uint32_t GetPrefilteredViewIndex(uint32_t _faceIndex, uint32_t _mipLevel) const;

    private:

        cVulkanImage m_captureImage;
        cVulkanImage m_prefilteredImage;

        std::array<VkImageView, 6> m_captureFaceImageViews;
        std::vector<VkImageView> m_prefilteredFaceMipImageViews;

        VkSampler m_sampler;

        uint32_t m_resolution;
        uint32_t m_mipLevels;
    };
}