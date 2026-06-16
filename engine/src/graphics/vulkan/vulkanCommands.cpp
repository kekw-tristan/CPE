#include "vulkanCommands.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <stdexcept>
#include <iostream>


// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanCommands::Init(const cVulkanDevice &_rDevice)
    {
        CreateCommandPool(_rDevice);
        AllocateCommandBuffer(_rDevice);

        std::cout << "Vulkan command pool and command buffer created" << std::endl;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanCommands::Shutdown(const cVulkanDevice &_rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_pCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, m_pCommandPool, nullptr);
            m_pCommandPool   = VK_NULL_HANDLE; 
            m_pCommandBuffer = VK_NULL_HANDLE;
        }
    }
    
    // -------------------------------------------------------------------------------------------------------------------------

    VkCommandPool cVulkanCommands::GetCommandPool() const
    {
        return m_pCommandPool;
    }
    
    // -------------------------------------------------------------------------------------------------------------------------

    VkCommandBuffer cVulkanCommands::GetCommandBuffer() const
    {
        return m_pCommandBuffer;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkCommandBuffer cVulkanCommands::BeginSingleTimeCommands(const cVulkanDevice& _rDevice)
    {
        VkCommandBufferAllocateInfo allocInfo{};

        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_pCommandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer pCommandBuffer = VK_NULL_HANDLE;

        if (vkAllocateCommandBuffers(_rDevice.GetDevice(), &allocInfo, &pCommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate single time command buffer!");
        }

        VkCommandBufferBeginInfo beginInfo{};

        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(pCommandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin single time command buffer");
        }

        return pCommandBuffer;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanCommands::EndSingleTimeCommands(const cVulkanDevice& _rDevice, VkCommandBuffer _pCommandBuffer)
    {
        if (vkEndCommandBuffer(_pCommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to end single time command buffer");
        }

        VkSubmitInfo submitInfo{};

        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &_pCommandBuffer;

        if (vkQueueSubmit(_rDevice.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit single time command buffer!");
        }

        vkQueueWaitIdle(_rDevice.GetGraphicsQueue());

        vkFreeCommandBuffers(_rDevice.GetDevice(), m_pCommandPool, 1, &_pCommandBuffer);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanCommands::CreateCommandPool(const cVulkanDevice &_rDevice)
    {
        VkCommandPoolCreateInfo poolInfo{};
        
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = _rDevice.GetQueueFamilyIndices().graphicsFamily;

        VkResult result = vkCreateCommandPool(_rDevice.GetDevice(), &poolInfo, nullptr, &m_pCommandPool); 

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan command pool!");
        }
    }
    
    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanCommands::AllocateCommandBuffer(const cVulkanDevice &_rDevice)
    {
        VkCommandBufferAllocateInfo allocInfo{}; 

        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_pCommandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1; 

        VkResult result = vkAllocateCommandBuffers(_rDevice.GetDevice(), &allocInfo, &m_pCommandBuffer); 

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate Vulkan command buffer!");
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------