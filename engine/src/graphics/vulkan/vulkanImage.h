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

            void Create(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height, VkFormat _format, VkImageUsageFlags _usage, VkImageAspectFlags _aspect, VkSampleCountFlagBits _samples, uint32_t _arrayLayers = 1, VkImageViewType _viewType = VK_IMAGE_VIEW_TYPE_2D, VkImageCreateFlags _flags = 0, uint32_t _mipLevels = 1);
            void Destroy(cVulkanDevice& _rDevice);
            void TransitionLayout(cVulkanDevice& _rDevice, VkCommandBuffer _pCommands, VkImageLayout _oldLayout, VkImageLayout _newLayout, VkImageAspectFlags _aspect);
            void CopyFromBuffer(VkCommandBuffer _pCommands, VkBuffer _sourceBuffer, VkDeviceSize _layerSize, VkImageAspectFlags _aspect);
            void GenerateMipmaps(cVulkanDevice& _rDevice, VkCommandBuffer _pCommands);
            void CopyMipChainFromBuffer(VkCommandBuffer _pCommands, VkBuffer _sourceBuffer, VkDeviceSize _bytesPerPixel, VkImageAspectFlags _aspect);

        public:

            VkImageView GetImageView()  const;
            VkImage     GetImage()      const;
            VkFormat    GetFormat()     const;
            uint32_t    GetMipLevels()  const;

            uint32_t GetWidth() const; 
            uint32_t GetHeight() const; 

        private:

            VkImage         m_image;
            VkDeviceMemory  m_memory;
            VkImageView     m_imageView;

            VkFormat        m_format;

            uint32_t        m_width;
            uint32_t        m_height;

            uint32_t        m_arrayLayers;

            uint32_t        m_mipLevels;
    };
}

