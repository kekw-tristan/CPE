#include "vulkanSync.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <iostream>
#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{
    
    // -------------------------------------------------------------------------------------------------------------------------
    
    void cVulkanSync::Init(const cVulkanDevice &_rDevice)
    {
        VkDevice device = _rDevice.GetDevice();


        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // fence starts signaled
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_pImageAvailableSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image available semaphore!");
        }

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_pRenderFinishedSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render finished semaphore!");
        }

        if (vkCreateFence(device, &fenceInfo, nullptr, &m_pInFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create in-flight fence!");
        }

        std::cout << "Vulkan sync objects created." << std::endl;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanSync::Shutdown(const cVulkanDevice &_rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_pInFlightFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(device, m_pInFlightFence, nullptr);
            m_pInFlightFence = VK_NULL_HANDLE;
        }

        if (m_pRenderFinishedSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, m_pRenderFinishedSemaphore, nullptr);
            m_pRenderFinishedSemaphore = VK_NULL_HANDLE;
        }

        if (m_pImageAvailableSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, m_pImageAvailableSemaphore, nullptr);
            m_pImageAvailableSemaphore = VK_NULL_HANDLE;
        }
    }
    

    // -------------------------------------------------------------------------------------------------------------------------

    VkSemaphore cVulkanSync::GetImageAvailableSemaphore() const
    {
        return m_pImageAvailableSemaphore;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkSemaphore cVulkanSync::GetRenderFinishedSemaphore() const
    {
        return m_pRenderFinishedSemaphore;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkFence cVulkanSync::GetInFlightFence() const
    {
        return m_pInFlightFence;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------