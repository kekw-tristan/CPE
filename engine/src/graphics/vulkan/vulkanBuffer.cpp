#include "vulkanBuffer.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <cstring>
#include <stdexcept>


// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBuffer::Create(cVulkanDevice &_rDevice, VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties)
    {
        m_size = _size;

        VkBufferCreateInfo bufferInfo{};

        bufferInfo.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size         = _size;
        bufferInfo.usage        = _usage;
        bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(_rDevice.GetDevice(), &bufferInfo, nullptr, &m_pBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan Buffer!");
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(_rDevice.GetDevice(), m_pBuffer, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo{}; 

        allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext             = nullptr;
        allocInfo.allocationSize    = memoryRequirements.size;
        allocInfo.memoryTypeIndex   = _rDevice.FindMemoryType(memoryRequirements.memoryTypeBits, _properties);

        if (vkAllocateMemory(_rDevice.GetDevice(), &allocInfo, nullptr, &m_pMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate Vulkan buffer memory!");
        }

        if (vkBindBufferMemory(_rDevice.GetDevice(), m_pBuffer, m_pMemory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bin Vulkan buffer memory!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBuffer::Shutdown(cVulkanDevice &_rDevice)
    {
        VkDevice device = _rDevice.GetDevice();

        if (m_pMappedData)
        {
            vkUnmapMemory(device, m_pMemory);
            m_pMappedData = nullptr;
        }

        if (m_pBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, m_pBuffer, nullptr);
            m_pBuffer = VK_NULL_HANDLE;
        }

        if (m_pMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_pMemory, nullptr);
            m_pMemory = VK_NULL_HANDLE;
        }

        m_size = 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBuffer::Map(cVulkanDevice &_rDevice, VkDeviceSize _size, VkDeviceSize _offset)
    {
        if (m_pMemory == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Cannot map invalid Vulkan buffer memory!");
        }

        if (vkMapMemory(_rDevice.GetDevice(), m_pMemory, _offset, _size, 0, &m_pMappedData) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map Vulkan buffer memory!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBuffer::Unmap(cVulkanDevice &_rDevice)
    {
        if (m_pMappedData)
        {
            vkUnmapMemory(_rDevice.GetDevice(), m_pMemory);
            m_pMappedData = nullptr;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBuffer::Write(const void *_pData, VkDeviceSize _size, VkDeviceSize _offset)
    {
        if (!m_pMappedData)
        {
            throw std::runtime_error("Cannot write to unmapped Vulkan buffer!");
        }

        std::memcpy(static_cast<char*>(m_pMappedData) + _offset, _pData, static_cast<size_t>(_size));
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkBuffer cVulkanBuffer::GetBuffer() const
    {
        return m_pBuffer;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkDeviceMemory cVulkanBuffer::GetMemory() const
    {
        return m_pMemory;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkDeviceSize cVulkanBuffer::GetSize() const
    {
        return m_size;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------