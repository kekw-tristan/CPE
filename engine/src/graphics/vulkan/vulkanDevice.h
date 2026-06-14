#pragma once 

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Engine::GFX
{
    class cVulkanContext;

    struct sQueueFamilyIndices
    {
        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily  = UINT32_MAX;

        bool IsComplete() const
        {
            return graphicsFamily != UINT32_MAX &&
                    presentFamily != UINT32_MAX;
        }
    };

    class cVulkanDevice
    {
        public:

            cVulkanDevice() = default;
           ~cVulkanDevice() = default;

            cVulkanDevice(const cVulkanDevice&)             = delete;
            cVulkanDevice& operator=(const cVulkanDevice&)  = delete;

        public:

            void Init(const cVulkanContext& _rContext);
            void Shutdown(); 

            VkPhysicalDevice            GetDevice() const;
            const sQueueFamilyIndices&  GetQueueFamilyIndices() const;

        private:

            void PickPhysicalDevice(const cVulkanContext& _rContext);
            bool IsDeviceSuitable(VkPhysicalDevice _pDevice, VkSurfaceKHR _pSurface);
            sQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice _pDevice, VkSurfaceKHR);

        private:

            VkPhysicalDevice    m_pPhysicalDevice;
            sQueueFamilyIndices m_queuFamilyIndices;
    };
}