#include "vulkanMesh.h"

#include "graphics/vulkan/vulkanDevice.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanMesh::Create(cVulkanDevice &_rDevice, const std::vector<sVulkanVertex> &_rVertices, const std::vector<uint32_t> &_rIndices)
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

        VkDeviceSize vertexBufferSize = sizeof(_rVertices[0]) * _rVertices.size();

        m_vertexBuffer.Create(_rDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_vertexBuffer.Map(_rDevice);
        m_vertexBuffer.Write(_rVertices.data(), vertexBufferSize);
        m_vertexBuffer.Unmap(_rDevice);

        VkDeviceSize indexBufferSize = sizeof(_rIndices[0]) * _rIndices.size();

        m_indexBuffer.Create(_rDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_indexBuffer.Map(_rDevice);
        m_indexBuffer.Write(_rIndices.data(), indexBufferSize);
        m_indexBuffer.Unmap(_rDevice);
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
        return false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------