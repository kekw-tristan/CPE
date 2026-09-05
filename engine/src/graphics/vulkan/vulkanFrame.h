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

        cVulkanBuffer healthBarBuffer;
        uint32_t healthBarCount = 0;

        cVulkanBuffer instanceBuffer;
        cVulkanBuffer instanceBufferStaging;

        cVulkanBuffer lightBuffer; 
        cVulkanBuffer lightStagingBuffer;

        cVulkanBuffer shadowBuffer;
        cVulkanBuffer shadowStagingBuffer;

    };
}
