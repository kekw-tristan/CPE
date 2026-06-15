#pragma once 

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;

    class cVulkanBuffer
    {
        public:

            cVulkanBuffer() = default;
           ~cVulkanBuffer() = default;

            cVulkanBuffer(const cVulkanBuffer&)             = delete;
            cVulkanBuffer& operator=(const cVulkanBuffer&)  = delete;

        public:

            void Create(cVulkanDevice& _rDevice, VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties);
            void Shutdown(cVulkanDevice& _rDevice);

            void Map(cVulkanDevice& _rDevice, VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
            void Unmap(cVulkanDevice& _rDevice);

            void Write(const void* _pData, VkDeviceSize _size, VkDeviceSize _offset = 0);

        public:

            VkBuffer        GetBuffer() const; 
            VkDeviceMemory  GetMemory() const; 
            VkDeviceSize    GetSize()   const;

            bool IsValid() const;

        private:

            VkBuffer        m_pBuffer;
            VkDeviceMemory  m_pMemory;
            VkDeviceSize    m_size;
            void*           m_pMappedData; 
    };
}