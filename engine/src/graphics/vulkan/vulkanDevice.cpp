#include "graphics/vulkan/vulkanDevice.h"

#include "graphics/vulkan/vulkanContext.h"

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept> 
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{
    // -------------------------------------------------------------------------------------------------------------------------

    static const std::vector<const char*> c_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDevice::Init(const cVulkanContext& _rContext)
    {
        PickPhysicalDevice(_rContext);
        CreateLogicalDevice();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDevice::Shutdown()
    {
        if (m_pDevice != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_pDevice, nullptr);
            m_pDevice = VK_NULL_HANDLE;
        }

        m_pPhysicalDevice   = VK_NULL_HANDLE; 
        m_pGraphicsQueue    = VK_NULL_HANDLE;
        m_pPresentQueue     = VK_NULL_HANDLE; 
        
        m_queueFamilyIndices = {};
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkPhysicalDevice cVulkanDevice::GetPhysicalDevice() const
    {
        return m_pPhysicalDevice;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkDevice cVulkanDevice::GetDevice() const
    {
        return m_pDevice;
    } 

    // -------------------------------------------------------------------------------------------------------------------------

    VkQueue cVulkanDevice::GetGraphicsQueue() const
    {
        return m_pGraphicsQueue;    
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkQueue cVulkanDevice::GetPresentQueue() const
    {
        return m_pPresentQueue;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sQueueFamilyIndices&  cVulkanDevice::GetQueueFamilyIndices() const
    {
        return m_queueFamilyIndices;
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
                m_queueFamilyIndices = FindQueueFamilies(device, _rContext.GetSurface());

                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(device, &properties);

                std::cout << "Selected GPU: " << properties.deviceName << std::endl;
                return;
            }
        }

        throw std::runtime_error("Failed to find a suitable Vulkan GPU.");
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanDevice::CreateLogicalDevice()
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos; 

        std::set<uint32_t> uniqueQueueFamilies = 
        { 
            m_queueFamilyIndices.graphicsFamily, 
            m_queueFamilyIndices.presentFamily 
        };

        float queuePriority = 1.f;

        for (uint32_t queueFamily: uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{}; 
            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos        = queueCreateInfos.data();
        createInfo.pEnabledFeatures         = &deviceFeatures;
        createInfo.enabledExtensionCount    = static_cast<uint32_t>(c_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames  = c_deviceExtensions.data();

        VkResult result = vkCreateDevice(m_pPhysicalDevice, &createInfo, nullptr, &m_pDevice);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan logical device!");
        }

        vkGetDeviceQueue(m_pDevice, m_queueFamilyIndices.graphicsFamily, 0, &m_pGraphicsQueue);
        vkGetDeviceQueue(m_pDevice, m_queueFamilyIndices.presentFamily,  0, &m_pPresentQueue);

        std::cout << "Vulkan logical device created" << std::endl;
    } 
    
    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanDevice::IsDeviceSuitable(VkPhysicalDevice _pDevice, VkSurfaceKHR _pSurface)
    {
        sQueueFamilyIndices indices = FindQueueFamilies(_pDevice, _pSurface);

        bool isExtensionSupported = CheckDeviceExtensionSupport(_pDevice);

        return indices.IsComplete() && isExtensionSupported;
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

    bool cVulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice _pDevice)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(_pDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);

        vkEnumerateDeviceExtensionProperties(_pDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(c_deviceExtensions.begin(), c_deviceExtensions.end());

        for (const VkExtensionProperties& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    } 
    
    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------