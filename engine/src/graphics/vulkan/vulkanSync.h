#pragma once

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;

    class cVulkanSync
    {
        public:
        
            cVulkanSync()   = default;
            ~cVulkanSync()  = default;

            cVulkanSync(const cVulkanSync&)             = delete;
            cVulkanSync& operator=(const cVulkanSync&)  = delete;

        public:

            void Init    (const cVulkanDevice& _rDevice);
            void Shutdown(const cVulkanDevice& _rDevice);

            VkSemaphore GetImageAvailableSemaphore() const;
            VkSemaphore GetRenderFinishedSemaphore() const;

            VkFence GetInFlightFence() const;

        private:
        
            VkSemaphore m_pImageAvailableSemaphore;
            VkSemaphore m_pRenderFinishedSemaphore;

            VkFence m_pInFlightFence;
    };
}