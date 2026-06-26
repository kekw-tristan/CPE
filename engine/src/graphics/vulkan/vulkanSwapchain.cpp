#include "graphics/vulkan/vulkanSwapchain.h"

#include "graphics/vulkan/vulkanContext.h"
#include "graphics/vulkan/vulkanDevice.h"

#include "platform/window.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "vulkanSwapchain.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{
    // -------------------------------------------------------------------------------------------------------------------------
    
    void cVulkanSwapchain::Init(const cVulkanContext&_rVulkanContext, const cVulkanDevice& _rDevice, const Platform::cWindow& _rWindow)
    {
        CreateSwapchain(_rVulkanContext, _rDevice, _rWindow);
        CreateImageViews(_rDevice);

        std::cout << "swapchain created" << std::endl;
    }
    
    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanSwapchain::Shutdown(const cVulkanDevice& _rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        for (VkImageView imageView : m_imageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        m_imageViews.clear();

        if (m_pSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, m_pSwapchain, nullptr);
            m_pSwapchain = VK_NULL_HANDLE;
        }

        m_images.clear();

        m_imageFormat   = VK_FORMAT_UNDEFINED;
        m_extent        = {};
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanSwapchain::Recreate(cVulkanContext &_rContext, cVulkanDevice &_rDevice, Engine::Platform::cWindow &_rWindow)
    {
        _rWindow.WaitUntilFramebufferHasSize(); 

        _rDevice.WaitIdle(); 
        
        Shutdown(_rDevice);

        Init(_rContext, _rDevice, _rWindow);
    } 
    
    // -------------------------------------------------------------------------------------------------------------------------

    VkSwapchainKHR cVulkanSwapchain::GetSwapchain() const
    {
        return m_pSwapchain;
    }

    // -------------------------------------------------------------------------------------------------------------------------
    
    VkFormat cVulkanSwapchain::GetImageFormat() const
    {
        return m_imageFormat;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkExtent2D cVulkanSwapchain::GetExtent() const
    {
        return m_extent;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanSwapchain::GetImageCount() const
    {
        return m_images.size();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const std::vector<VkImage> &cVulkanSwapchain::GetImages() const
    {
        return m_images;
    }

    // -------------------------------------------------------------------------------------------------------------------------


    const std::vector<VkImageView> &cVulkanSwapchain::GetImageViews() const
    {
        return m_imageViews;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanSwapchain::CreateSwapchain(const cVulkanContext &_rContext, const cVulkanDevice &_rDevice, const Platform::cWindow &_rWindow)
    {
        sSwapchainSupportDetails support = QuerySwapchainSupport(_rDevice.GetPhysicalDevice(), _rContext.GetSurface());

        VkSurfaceFormatKHR  surfaceFormat   = ChooseSurfaceFormat(support.formats);
        VkPresentModeKHR    presentMode     = ChoosePresentMode(support.presentModes);
        VkExtent2D          extent          = ChooseExtent(support.capabilities, _rWindow);

        uint32_t imageCount = support.capabilities.minImageCount + 1; 

        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
        {
            imageCount = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};

        createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface          = _rContext.GetSurface();
        createInfo.minImageCount    = imageCount;
        createInfo.imageFormat      = surfaceFormat.format;
        createInfo.imageColorSpace  = surfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const sQueueFamilyIndices& indices = _rDevice.GetQueueFamilyIndices();

        uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount    = 2;
            createInfo.pQueueFamilyIndices      = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount    = 0;
            createInfo.pQueueFamilyIndices      = nullptr;
        }

        createInfo.preTransform     = support.capabilities.currentTransform;
        createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode      = presentMode;
        createInfo.clipped          = VK_TRUE;
        createInfo.oldSwapchain     = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(_rDevice.GetDevice(), &createInfo, nullptr, &m_pSwapchain);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan swapchain!");
        }

        vkGetSwapchainImagesKHR(_rDevice.GetDevice(), m_pSwapchain, &imageCount, nullptr);

        m_images.resize(imageCount);

        vkGetSwapchainImagesKHR(_rDevice.GetDevice(), m_pSwapchain, &imageCount, m_images.data());

        m_imageFormat = surfaceFormat.format;
        m_extent      = extent;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanSwapchain::CreateImageViews(const cVulkanDevice &_rDevice)
    {
        m_imageViews.resize(m_images.size());

        for (size_t index = 0; index < m_images.size(); ++index)
        {
            VkImageViewCreateInfo createInfo{};

            createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image    = m_images[index];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format   = m_imageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel    = 0;
            createInfo.subresourceRange.levelCount      = 1;
            createInfo.subresourceRange.baseArrayLayer  = 0;
            createInfo.subresourceRange.layerCount      = 1;

            VkResult result = vkCreateImageView(_rDevice.GetDevice(), &createInfo, nullptr, &m_imageViews[index]);

            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create swapchain image view!");
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sSwapchainSupportDetails cVulkanSwapchain::QuerySwapchainSupport(VkPhysicalDevice _pPhysicalDevice, VkSurfaceKHR _pSurface) const
    {
        sSwapchainSupportDetails details{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_pPhysicalDevice, _pSurface, &details.capabilities);

        uint32_t formatCount = 0; 
        vkGetPhysicalDeviceSurfaceFormatsKHR(_pPhysicalDevice, _pSurface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(_pPhysicalDevice, _pSurface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0; 
        vkGetPhysicalDeviceSurfacePresentModesKHR(_pPhysicalDevice, _pSurface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);

            vkGetPhysicalDeviceSurfacePresentModesKHR(_pPhysicalDevice, _pSurface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkSurfaceFormatKHR cVulkanSwapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &_rAvailableFormats) const
    {
        for (const VkSurfaceFormatKHR& availableFormat : _rAvailableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return _rAvailableFormats[0];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkPresentModeKHR cVulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR> &_rAvailablePresentModes) const
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkExtent2D cVulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR &_rCapabilities, const Platform::cWindow &_rWindow) const
    {
        if (_rCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return _rCapabilities.currentExtent;
        }

        int width  = 0;
        int height = 0;

        glfwGetFramebufferSize(_rWindow.GetWindow(), &width, &height);

        VkExtent2D actualExtent = 
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width  = std::clamp(actualExtent.width,  _rCapabilities.minImageExtent.width,  _rCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, _rCapabilities.minImageExtent.height, _rCapabilities.maxImageExtent.height);

        return actualExtent;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------