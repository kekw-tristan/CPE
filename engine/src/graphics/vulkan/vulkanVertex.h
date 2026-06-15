#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>

namespace Engine::GFX
{
    struct sVulkanVertex
    {
        float position [3];
        float normal   [3];
        float texCoord [2];
        float color    [4];    

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding      = 0;
            bindingDescription.stride       = sizeof(sVulkanVertex);
            bindingDescription.inputRate    = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

            attributeDescriptions[0].binding    = 0;
            attributeDescriptions[0].location   = 0;
            attributeDescriptions[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset     = offsetof(sVulkanVertex, position);

            attributeDescriptions[1].binding    = 0;
            attributeDescriptions[1].location   = 1;
            attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset     = offsetof(sVulkanVertex, normal);

            attributeDescriptions[2].binding    = 0;
            attributeDescriptions[2].location   = 2;
            attributeDescriptions[2].format     = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset     = offsetof(sVulkanVertex, texCoord);

            attributeDescriptions[3].binding    = 0;
            attributeDescriptions[3].location   = 3;
            attributeDescriptions[3].format     = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[3].offset     = offsetof(sVulkanVertex, color);

            return attributeDescriptions;
        }
    };
}