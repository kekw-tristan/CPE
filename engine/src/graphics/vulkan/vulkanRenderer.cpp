#include "vulkanRenderer.h"

#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanCommands.h"
#include "graphics/vulkan/vulkanSync.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanSync& _rSync)
    {
        m_pDevice    = &_rDevice;
        m_pSwapchain = &_rSwapChain;
        m_pCommands  = &_rCommands;
        m_pSync      = &_rSync; 
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawFrame()
    {
        VkDevice device        = m_pDevice->GetDevice();
        VkFence  inFlightFence = m_pSync->GetInFlightFence();

        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX); 
        vkResetFences(device, 1, &inFlightFence);

        uint32_t imageIndex = 0; 

        VkResult acqureResult = vkAcquireNextImageKHR(device, 
                                                      m_pSwapchain->GetSwapchain(),
                                                      UINT64_MAX,m_pSync->GetImageAvailableSemaphore(),
                                                      VK_NULL_HANDLE,
                                                      &imageIndex);

        if (acqureResult != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        VkCommandBuffer commandBuffer = m_pCommands->GetCommandBuffer(); 

        vkResetCommandBuffer(commandBuffer, 0); 
        RecordCommandBuffer(imageIndex);

        VkSemaphore          waitSemaphores[]   = { m_pSync->GetImageAvailableSemaphore() };
        VkSemaphore          signalSemaphores[] = { m_pSync->GetRenderFinishedSemaphore() };
        VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};

        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, m_pSync->GetInFlightFence());

        if (submitResult != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkSwapchainKHR swapchains[] = { m_pSwapchain->GetSwapchain() };

         VkPresentInfoKHR presentInfo{};

        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = signalSemaphores;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = swapchains;
        presentInfo.pImageIndices       = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_pDevice->GetPresentQueue(), &presentInfo);

        if (presentResult != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain image.");
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::RecordCommandBuffer(uint32_t _imageIndex)
    {
        VkCommandBuffer commandBuffer = m_pCommands->GetCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{}; 
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo)!= VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkImage swapchainImage = m_pSwapchain->GetImages()[_imageIndex];

        VkImageMemoryBarrier barrierToClear{};
        barrierToClear.sType                            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToClear.oldLayout                        = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierToClear.newLayout                        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierToClear.srcQueueFamilyIndex              = VK_QUEUE_FAMILY_IGNORED;
        barrierToClear.dstQueueFamilyIndex              = VK_QUEUE_FAMILY_IGNORED;
        barrierToClear.image                            = swapchainImage;
        barrierToClear.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierToClear.subresourceRange.baseMipLevel    = 0;
        barrierToClear.subresourceRange.levelCount      = 1;
        barrierToClear.subresourceRange.baseArrayLayer  = 0;
        barrierToClear.subresourceRange.layerCount      = 1;
        barrierToClear.srcAccessMask                    = 0;
        barrierToClear.dstAccessMask                    = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToClear);

        VkClearColorValue clearColor{};

        clearColor.float32[0] = 0.0f;
        clearColor.float32[1] = 0.0f;
        clearColor.float32[2] = 0.0f;
        clearColor.float32[3] = 1.0f;

        VkImageSubresourceRange clearRange{};

        clearRange.aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
        clearRange.baseMipLevel     = 0;
        clearRange.levelCount       = 1;
        clearRange.baseArrayLayer   = 0;
        clearRange.layerCount       = 1;

        vkCmdClearColorImage(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &clearRange);

        VkImageMemoryBarrier barrierToPresent{};

        barrierToPresent.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToPresent.oldLayout                          = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierToPresent.newLayout                          = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrierToPresent.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToPresent.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToPresent.image                              = swapchainImage;
        barrierToPresent.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierToPresent.subresourceRange.baseMipLevel      = 0;
        barrierToPresent.subresourceRange.levelCount        = 1;
        barrierToPresent.subresourceRange.baseArrayLayer    = 0;
        barrierToPresent.subresourceRange.layerCount        = 1;
        barrierToPresent.srcAccessMask                      = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrierToPresent.dstAccessMask                      = 0;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToPresent);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------