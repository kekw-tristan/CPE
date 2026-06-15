#include "vulkanRenderer.h"

#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanPipeline.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanCommands.h"
#include "graphics/vulkan/vulkanSync.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanSync& _rSync, cVulkanPipeline& _rPipeline)
    {
        m_pDevice    = &_rDevice;
        m_pSwapchain = &_rSwapChain;
        m_pCommands  = &_rCommands;
        m_pSync      = &_rSync; 
        m_pPipeline  = &_rPipeline;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanRenderer::DrawFrame()
    {
        VkDevice device        = m_pDevice->GetDevice();
        VkFence  inFlightFence = m_pSync->GetInFlightFence();

        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX); 
        

        uint32_t imageIndex = 0; 

        VkResult acquireResult = vkAcquireNextImageKHR(device, 
                                                      m_pSwapchain->GetSwapchain(),
                                                      UINT64_MAX,m_pSync->GetImageAvailableSemaphore(),
                                                      VK_NULL_HANDLE,
                                                      &imageIndex);

         if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return false;
        }

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        VkCommandBuffer commandBuffer = m_pCommands->GetCommandBuffer(); 

        vkResetCommandBuffer(commandBuffer, 0); 
        RecordCommandBuffer(imageIndex);

        vkResetFences(device, 1, &inFlightFence);

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

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            return false;
        }

        if (presentResult != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain image.");
        }

        return true;
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

        VkImage     swapchainImage     = m_pSwapchain->GetImages()[_imageIndex];
        VkImageView swapchainImageView = m_pSwapchain->GetImageViews()[_imageIndex];
        VkExtent2D  extent             = m_pSwapchain->GetExtent();

        VkImageMemoryBarrier barrierToColorAttachment{};

        barrierToColorAttachment.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToColorAttachment.oldLayout                          = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierToColorAttachment.newLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrierToColorAttachment.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToColorAttachment.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToColorAttachment.image                              = swapchainImage;
        barrierToColorAttachment.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierToColorAttachment.subresourceRange.baseMipLevel      = 0;
        barrierToColorAttachment.subresourceRange.levelCount        = 1;
        barrierToColorAttachment.subresourceRange.baseArrayLayer    = 0;
        barrierToColorAttachment.subresourceRange.layerCount        = 1;
        barrierToColorAttachment.srcAccessMask                      = 0;
        barrierToColorAttachment.dstAccessMask                      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierToColorAttachment
        );

        VkClearValue clearValue{};

        clearValue.color.float32[0] = 0.0f;
        clearValue.color.float32[1] = 0.0f;
        clearValue.color.float32[2] = 0.0f;
        clearValue.color.float32[3] = 1.0f;

        VkRenderingAttachmentInfo colorAttachment{};

        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = swapchainImageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue  = clearValue;

        VkRenderingInfo renderingInfo{};

        renderingInfo.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset     = { 0, 0 };
        renderingInfo.renderArea.extent     = extent;
        renderingInfo.layerCount            = 1;
        renderingInfo.colorAttachmentCount  = 1;
        renderingInfo.pColorAttachments     = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pPipeline->GetPipeline()
        );

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);

        VkImageMemoryBarrier barrierToPresent{};

        barrierToPresent.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToPresent.oldLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrierToPresent.newLayout                          = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrierToPresent.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToPresent.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToPresent.image                              = swapchainImage;
        barrierToPresent.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierToPresent.subresourceRange.baseMipLevel      = 0;
        barrierToPresent.subresourceRange.levelCount        = 1;
        barrierToPresent.subresourceRange.baseArrayLayer    = 0;
        barrierToPresent.subresourceRange.layerCount        = 1;
        barrierToPresent.srcAccessMask                      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrierToPresent.dstAccessMask                      = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierToPresent
        );

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------