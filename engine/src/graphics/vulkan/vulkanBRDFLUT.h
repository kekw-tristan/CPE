#pragma once

#include "graphics/vulkan/vulkanImage.h"

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;
    class cVulkanCommands;

    class cVulkanBRDFLUT
    {
    public:

        cVulkanBRDFLUT();
        ~cVulkanBRDFLUT();

        cVulkanBRDFLUT(const cVulkanBRDFLUT&) = delete;
        cVulkanBRDFLUT& operator=(const cVulkanBRDFLUT&) = delete;

    public:

        void Create(cVulkanDevice& _rDevice, cVulkanCommands& _rCommands);
        void Destroy(cVulkanDevice& _rDevice);

    public:

        VkImageView GetImageView() const;
        VkSampler   GetSampler() const;

    private:

        cVulkanImage m_image;
        VkSampler    m_sampler;
    };
}