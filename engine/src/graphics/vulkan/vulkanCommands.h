#pragma once

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;

    class cVulkanCommands
    {
        
        public:

            cVulkanCommands()   = default;
            ~cVulkanCommands()  = default;

            cVulkanCommands(const cVulkanCommands&)             = delete;
            cVulkanCommands& operator=(const cVulkanCommands&)  = delete;

        public:

            void Init    (const cVulkanDevice& _rDevice);
            void Shutdown(const cVulkanDevice& _rDevice);

            VkCommandPool   GetCommandPool()   const;
            VkCommandBuffer GetCommandBuffer() const;

        private:

            void CreateCommandPool    (const cVulkanDevice& _rDevice);
            void AllocateCommandBuffer(const cVulkanDevice& _rDevice);

        private:

            VkCommandPool   m_pCommandPool   = VK_NULL_HANDLE;
            VkCommandBuffer m_pCommandBuffer = VK_NULL_HANDLE;
    };
}