#pragma once

#include <graphics/vulkan/vulkanBuffer.h>

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    struct sVulkanFrame
    {
        VkCommandBuffer pCommandBuffer = VK_NULL_HANDLE;
        VkFence         inFlightFence  = VK_NULL_HANDLE;
        
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;

        cVulkanBuffer frameUniformedBuffer;
        VkDescriptorSet frameDescriptorSet = VK_NULL_HANDLE;

        cVulkanBuffer instanceBuffer;
        cVulkanBuffer instanceBufferStaging;
    };
}
