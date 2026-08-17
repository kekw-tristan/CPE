#pragma once

#include "graphics/vulkan/vulkanImage.h"

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;
    class cVulkanCommands;

    constexpr uint32_t c_irradianceSize = 32;

    class cVulkanEnvironment
    {

        public:

            cVulkanEnvironment();
            ~cVulkanEnvironment();

            cVulkanEnvironment(const cVulkanEnvironment&)               = delete;
            cVulkanEnvironment& operator=(const cVulkanEnvironment&)    = delete;

        public:

            void Create(cVulkanDevice& _rDevice, cVulkanCommands& _rCommands);
            void Destroy(cVulkanDevice& _rDevice);

        public:

            VkImageView GetImageView() const;
            VkImageView GetIrradianceImageView() const;
            VkSampler   GetSampler() const;

        private:

            cVulkanImage m_environmentImage;
            cVulkanImage m_irradianceImage;

            VkSampler m_sampler;
    };
}