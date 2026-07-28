#pragma once 

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;

    class cVulkanImage
    {
        public:

            cVulkanImage();
           ~cVulkanImage();

            cVulkanImage(const cVulkanImage&)               = delete;
            cVulkanImage& operator=(const cVulkanImage&)    = delete;

        public:

            void Create(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height, VkFormat _format, VkImageUsageFlags _usage, VkImageAspectFlags _aspect, VkSampleCountFlagBits _samples);
            void Destroy(cVulkanDevice& _rDevice);
            void TransitionLayout(cVulkanDevice& _rDevice, VkCommandBuffer _pCommands, VkImageLayout _oldLayout, VkImageLayout _newLayout, VkImageAspectFlags _aspect);

        public:

            VkImageView GetImageView()  const;
            VkImage     GetImage()      const;
            VkFormat    GetFormat()     const;

        private:

            VkImage         m_image;
            VkDeviceMemory  m_memory;
            VkImageView     m_imageView;

            VkFormat        m_format;

            uint32_t        m_width;
            uint32_t        m_height;
    };
}

