#pragma once 

#include "graphics/vulkan/vulkanBuffer.h"
#include "graphics/vulkan/vulkanVertex.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <cstdint>

namespace Engine::GFX
{
    class cVulkanDevice; 

    class cVulkanMesh
    {
        public:

            cVulkanMesh()   = default;
            ~cVulkanMesh()  = default;

            cVulkanMesh(const cVulkanMesh&)             = delete;
            cVulkanMesh& operator=(const cVulkanMesh&)  = delete;

        public:

            void Create(cVulkanDevice& _rDevice, const std::vector<sVulkanVertex>& _rVertices, const std::vector<uint32_t>& _rIndices);
            void Shutdown(cVulkanDevice& _rDevice); 
            void Draw(VkCommandBuffer _pCommandBuffer)const;
            bool IsValid() const;

        private:

            cVulkanBuffer m_vertexBuffer;
            cVulkanBuffer m_indexBuffer;

            uint32_t m_indexCount;

    };
}