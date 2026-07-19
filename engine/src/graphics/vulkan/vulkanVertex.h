#pragma once

#include "graphics/vertex.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>

namespace Engine::GFX
{
    struct sVulkanVertex
    {
        sVertex vertex;  

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding      = 0;
            bindingDescription.stride       = sizeof(sVulkanVertex);
            bindingDescription.inputRate    = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding    = 0;
            attributeDescriptions[0].location   = 0;
            attributeDescriptions[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset     = offsetof(sVulkanVertex, vertex.position);

            attributeDescriptions[1].binding    = 0;
            attributeDescriptions[1].location   = 1;
            attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset     = offsetof(sVulkanVertex, vertex.normal);

            attributeDescriptions[2].binding    = 0;
            attributeDescriptions[2].location   = 2;
            attributeDescriptions[2].format     = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset     = offsetof(sVulkanVertex, vertex.uv);

            return attributeDescriptions;
        }
    };
}