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

        public:

            void WaitIdle() const;

        public:

            VkPhysicalDevice    GetPhysicalDevice() const;
            VkDevice            GetDevice()         const; 

            VkQueue GetGraphicsQueue() const;
            VkQueue GetPresentQueue()  const;

            const sQueueFamilyIndices&  GetQueueFamilyIndices() const;

        private:

            void PickPhysicalDevice(const cVulkanContext& _rContext);
            void CreateLogicalDevice();

            bool IsDeviceSuitable(VkPhysicalDevice _pDevice, VkSurfaceKHR _pSurface);
            sQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice _pDevice, VkSurfaceKHR);
            bool CheckDeviceExtensionSupport(VkPhysicalDevice _pDevice);

        private:

            VkPhysicalDevice    m_pPhysicalDevice;
            VkDevice            m_pDevice;

            VkQueue m_pGraphicsQueue; 
            VkQueue m_pPresentQueue;

            sQueueFamilyIndices m_queueFamilyIndices;

    };
}