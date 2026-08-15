#include "graphics/vulkan/vulkanDevice.h"

#include "graphics/vulkan/vulkanContext.h"

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept> 
#include <vector>
#include "vulkanDevice.h"

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

    void cVulkanDevice::WaitIdle() const
    {
        if (m_pDevice != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_pDevice);
        }
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

    VkSampleCountFlagBits cVulkanDevice::GetMSAASamples() const
    {
        return m_msaaSamples;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sQueueFamilyIndices&  cVulkanDevice::GetQueueFamilyIndices() const
    {
        return m_queueFamilyIndices;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanDevice::FindMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties) const
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;

        vkGetPhysicalDeviceMemoryProperties(m_pPhysicalDevice, &memoryProperties);

        for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
        {
            bool typeMatches = (_typeFilter & (1 << index)) != 0;
            bool propertiesMatch = (memoryProperties.memoryTypes[index].propertyFlags & _properties) == _properties;
            
            if (typeMatches && propertiesMatch)
            {
                return index;
            }
        }

        throw std::runtime_error("Failed to find suitable Vulkan memory type!");
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
                m_pPhysicalDevice    = device;
                m_queueFamilyIndices = FindQueueFamilies(device, _rContext.GetSurface());
                m_msaaSamples        = GetMaxUsabelSampleCount();

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

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex    = queueFamily;
            queueCreateInfo.queueCount          = 1;
            queueCreateInfo.pQueuePriorities    = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(m_pPhysicalDevice, &deviceProperties);

        if (deviceProperties.apiVersion < VK_API_VERSION_1_3)
        {
            throw std::runtime_error("GPU does not support Vulkan 1.3!");
        }

        VkPhysicalDeviceVulkan13Features supportedVulkan13Features{};
        supportedVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        VkPhysicalDeviceFeatures2 supportedFeatures{};
        supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supportedFeatures.pNext = &supportedVulkan13Features;

        vkGetPhysicalDeviceFeatures2(m_pPhysicalDevice, &supportedFeatures);

        if (supportedVulkan13Features.dynamicRendering != VK_TRUE)
        {
            throw std::runtime_error("GPU does not support Vulkan dynamic rendering!");
        }

        VkPhysicalDeviceVulkan13Features vulkan13Features{};
        vulkan13Features.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        vulkan13Features.dynamicRendering   = VK_TRUE;

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos        = queueCreateInfos.data();
        createInfo.pEnabledFeatures         = &deviceFeatures;
        createInfo.enabledExtensionCount    = static_cast<uint32_t>(c_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = c_deviceExtensions.data();
        createInfo.pNext = &vulkan13Features;

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

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

        VkPhysicalDeviceFeatures2 features2{};

        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &dynamicRenderingFeatures;

        vkGetPhysicalDeviceFeatures2(_pDevice, &features2);

        return indices.IsComplete() && isExtensionSupported && dynamicRenderingFeatures.dynamicRendering;
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
    
    VkSampleCountFlagBits cVulkanDevice::GetMaxUsabelSampleCount()
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_pPhysicalDevice, &properties);

        VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
        if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
        if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
        if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
        if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
        if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

        return VK_SAMPLE_COUNT_1_BIT;
    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------