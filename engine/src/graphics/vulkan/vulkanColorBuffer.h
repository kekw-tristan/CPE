#pragma once

#include <graphics/vulkan/vulkanImage.h>

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;
    class cVulkanSwapchain;
    class cVulkanCommands;


    class cVulkanColorBuffer
    {

        public:

            cVulkanColorBuffer();
            ~cVulkanColorBuffer() = default;


            cVulkanColorBuffer(const cVulkanColorBuffer&) = delete;
            cVulkanColorBuffer& operator=(const cVulkanColorBuffer&) = delete;


        public:

            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapchain, cVulkanCommands& _rCommands);
            void ShutDown(cVulkanDevice& _rDevice);


        public:

            VkImageView GetImageView() const;
            VkFormat GetFormat() const;


        private:

            cVulkanImage m_image;

    };
}