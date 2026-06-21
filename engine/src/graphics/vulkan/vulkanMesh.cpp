#include "vulkanMesh.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanMesh::Create(cVulkanDevice& _rDevice, cVulkanCommands& _rCommands,const std::vector<sVertex>& _rVertices, const std::vector<uint32_t>& _rIndices)
    {
        if (_rVertices.empty())
        {
            throw std::runtime_error("Cannot create Vulkan mesh without vertices!");
        }

        if (_rIndices.empty())
        {
            throw std::runtime_error("Cannot create Vulkan mesh without indices!");
        }

        m_indexCount = static_cast<uint32_t>(_rIndices.size());


        // vertexBuffer
        VkDeviceSize  vertexBufferSize = sizeof(_rVertices[0]) * _rVertices.size();
        cVulkanBuffer vertexStaginBuffer;

        vertexStaginBuffer.Create(_rDevice, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vertexStaginBuffer.Map(_rDevice);
        vertexStaginBuffer.Write(_rVertices.data(), vertexBufferSize);
        vertexStaginBuffer.Unmap(_rDevice);

        m_vertexBuffer.Create(_rDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        cVulkanBuffer::CopyBuffer(_rDevice, _rCommands, vertexStaginBuffer.GetBuffer(), m_vertexBuffer.GetBuffer(), vertexBufferSize);
        vertexStaginBuffer.Shutdown(_rDevice);

        // Indexbuffer
        VkDeviceSize indexBufferSize = sizeof(_rIndices[0]) * _rIndices.size();
        cVulkanBuffer indexStagingBuffer;
        indexStagingBuffer.Create(_rDevice, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        indexStagingBuffer.Map(_rDevice);
        indexStagingBuffer.Write(_rIndices.data(), indexBufferSize);
        indexStagingBuffer.Unmap(_rDevice);

        m_indexBuffer.Create(_rDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        cVulkanBuffer::CopyBuffer(_rDevice, _rCommands, indexStagingBuffer.GetBuffer(), m_indexBuffer.GetBuffer(), indexBufferSize);
        indexStagingBuffer.Shutdown(_rDevice);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanMesh::Shutdown(cVulkanDevice &_rDevice)
    {
        m_indexBuffer .Shutdown(_rDevice);
        m_vertexBuffer.Shutdown(_rDevice);

        m_indexCount = 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanMesh::Draw(VkCommandBuffer _pCommandBuffer) const
    {
        VkBuffer vertexBuffers[]    = { m_vertexBuffer.GetBuffer() };
        VkDeviceSize offsets[]      = { 0 };

        vkCmdBindVertexBuffers  (_pCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer    (_pCommandBuffer, m_indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed        (_pCommandBuffer, m_indexCount, 1, 0, 0, 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanMesh::IsValid() const
    {
        return m_indexCount > 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------