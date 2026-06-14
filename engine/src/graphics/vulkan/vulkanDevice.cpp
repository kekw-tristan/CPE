#include "graphics/vulkan/vulkanDevice.h"

#include "graphics/vulkan/vulkanContext.h"

#include <iostream>
#include <stdexcept> 
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDevice::Init(const cVulkanContext& _rContext)
    {
        PickPhysicalDevice(_rContext);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDevice::Shutdown()
    {
        m_pPhysicalDevice   = VK_NULL_HANDLE; 
        m_queuFamilyIndices = {};
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkPhysicalDevice cVulkanDevice::GetDevice() const
    {
        return m_pPhysicalDevice;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sQueueFamilyIndices&  cVulkanDevice::GetQueueFamilyIndices() const
    {
        return m_queuFamilyIndices;
    }

    // -------------------------------------------------------------------------------------------------------------------------
    
    void cVulkanDevice::PickPhysicalDevice(const cVulkanContext& _rContext)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(_rContext.GetInstance(), &deviceCount, nullptr); 

        if (deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(_rContext.GetInstance(), &deviceCount, devices.data());

        for (VkPhysicalDevice device : devices)
        {
            if (IsDeviceSuitable(device, _rContext.GetSurface()))
            {
                m_pPhysicalDevice   = device;
                m_queuFamilyIndices = FindQueueFamilies(device, _rContext.GetSurface());

                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(device, &properties);

                std::cout << "Selected GPU: " << properties.deviceName << std::endl;
                return;
            }
        }

        throw std::runtime_error("Failed to find a suitable Vulkan GPU.");
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanDevice::IsDeviceSuitable(VkPhysicalDevice _pDevice, VkSurfaceKHR _pSurface)
    {
        sQueueFamilyIndices indices = FindQueueFamilies(_pDevice, _pSurface);
        return indices.IsComplete();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sQueueFamilyIndices cVulkanDevice::FindQueueFamilies(VkPhysicalDevice _pDevice, VkSurfaceKHR _pSurface)
    {
        sQueueFamilyIndices indices{};

        uint32_t queueFamilyCount = 0; 
        vkGetPhysicalDeviceQueueFamilyProperties(_pDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_pDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t index = 0; index < queueFamilyCount; ++index)
        {
            const VkQueueFamilyProperties& queueFamily = queueFamilies[index]; 

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = index; 
            }

            VkBool32 presentSupport = VK_FALSE;

            vkGetPhysicalDeviceSurfaceSupportKHR(_pDevice, index, _pSurface, &presentSupport);
            
            if (presentSupport)
            {
                indices.presentFamily =  index; 
            }

            if (indices.IsComplete())
            {
                break;
            }
        }

        return indices;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------