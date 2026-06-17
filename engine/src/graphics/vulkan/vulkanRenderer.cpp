#include "vulkanRenderer.h"

#include "graphics/camera.h"
#include "graphics/frameUniformData.h"
#include "graphics/gfxConfig.h"

#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanMesh.h"
#include "graphics/vulkan/vulkanPipeline.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanCommands.h"

#include <iostream>
#include <cstring>
#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanPipeline& _rPipeline, cVulkanMesh& _rMesh)
    {
        m_pDevice    = &_rDevice;
        m_pSwapchain = &_rSwapChain;
        m_pCommands  = &_rCommands;
        m_pPipeline  = &_rPipeline;
        m_pMesh      = &_rMesh;

        m_currentFrame = 0;

        m_depthBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);

        CreateFrameResources();
        CreateDescriptorPool();
        CreateDescriptorSets();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::ShutDown()
    {
        if (m_pDevice == nullptr)
        {
            return;
        }

        VkDevice device = m_pDevice->GetDevice(); 

        m_depthBuffer.ShutDown(*m_pDevice);

        for (sVulkanFrame& rFrame : m_frames)
        {
            if (rFrame.imageAvailableSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(device, rFrame.imageAvailableSemaphore, nullptr);
                rFrame.imageAvailableSemaphore = VK_NULL_HANDLE;
            }
            
            if (rFrame.renderFinishedSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(device, rFrame.renderFinishedSemaphore, nullptr);
                rFrame.renderFinishedSemaphore = VK_NULL_HANDLE;
            }

            if (rFrame.inFlightFence != VK_NULL_HANDLE)
            {
                vkDestroyFence(device, rFrame.inFlightFence, nullptr);
                rFrame.inFlightFence = VK_NULL_HANDLE;
            }

            if (rFrame.pCommandBuffer != VK_NULL_HANDLE && m_pCommands != nullptr)
            {
                vkFreeCommandBuffers(device, m_pCommands->GetCommandPool(), 1, &rFrame.pCommandBuffer);
                rFrame.pCommandBuffer = VK_NULL_HANDLE;
            }

            if (rFrame.frameUniformedBuffer.GetBuffer() != VK_NULL_HANDLE)
            {
                rFrame.frameUniformedBuffer.Shutdown(*m_pDevice);
            }
        }

        if (m_pDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, m_pDescriptorPool, nullptr);
            m_pDescriptorPool = VK_NULL_HANDLE;
        }

        m_currentFrame = 0;

        m_pMesh      = nullptr;
        m_pPipeline  = nullptr;
        m_pCommands  = nullptr;
        m_pSwapchain = nullptr;
        m_pDevice    = nullptr;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanRenderer::DrawFrame(const cCamera& _rCamera)
    {
        VkDevice        device          = m_pDevice->GetDevice();
        sVulkanFrame&   frame           = m_frames[m_currentFrame];

        vkWaitForFences(device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX); 
        

        uint32_t imageIndex = 0; 

        VkResult acquireResult = vkAcquireNextImageKHR(
            device, 
            m_pSwapchain->GetSwapchain(),
            UINT64_MAX,frame.imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return false;
        }

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        UpdateFrameUniformBuffer(frame, _rCamera);

        VkCommandBuffer commandBuffer = m_pCommands->GetCommandBuffer(); 

        vkResetCommandBuffer(commandBuffer, 0); 
        RecordCommandBuffer(commandBuffer, imageIndex, frame);

        vkResetFences(device, 1, &frame.inFlightFence);

        VkSemaphore          waitSemaphores[]   = { frame.imageAvailableSemaphore };
        VkSemaphore          signalSemaphores[] = { frame.renderFinishedSemaphore };
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

        VkResult submitResult = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, frame.inFlightFence);

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

    void cVulkanRenderer::RecreateDepthBuffer()
    {
        m_pDevice->WaitIdle();

        m_depthBuffer.ShutDown(*m_pDevice);

        m_depthBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::RecordCommandBuffer(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex, sVulkanFrame& _rFrame)
    {
        VkCommandBuffer commandBuffer = _pCommandBuffer;

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

        VkRenderingAttachmentInfo depthAttachment{};

        depthAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView               = m_depthBuffer.GetImageView();
        depthAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo renderingInfo{};

        renderingInfo.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset     = { 0, 0 };
        renderingInfo.renderArea.extent     = extent;
        renderingInfo.layerCount            = 1;
        renderingInfo.colorAttachmentCount  = 1;
        renderingInfo.pColorAttachments     = &colorAttachment;
        renderingInfo.pDepthAttachment      = &depthAttachment;
        renderingInfo.pStencilAttachment    = nullptr;

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

        vkCmdBindDescriptorSets(_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPipelineLayout(), 0, 1, &_rFrame.frameDescriptorSet, 0, nullptr);

        m_pMesh->Draw(commandBuffer);

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

    void cVulkanRenderer::CreateFrameResources()
    {
        VkDevice device = m_pDevice->GetDevice();


        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // fence starts signaled
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (sVulkanFrame& rFrame : m_frames)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &rFrame.imageAvailableSemaphore) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image available semaphore!");
            }
        
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &rFrame.renderFinishedSemaphore) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create render finished semaphore!");
            }
        
            if (vkCreateFence(device, &fenceInfo, nullptr, &rFrame.inFlightFence) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create in-flight fence!");
            }

            rFrame.frameUniformedBuffer.Create(*m_pDevice, sizeof(sFrameUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
            rFrame.frameUniformedBuffer.Map(*m_pDevice, sizeof(sFrameUniformData), 0);
        }

        std::cout << "Vulkan sync objects created." << std::endl;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{};
        
        poolSize.type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount    = c_maxNumberOfFrames;

        VkDescriptorPoolCreateInfo poolInfo{};

        poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount  = 1;
        poolInfo.pPoolSizes     = &poolSize;
        poolInfo.maxSets        = c_maxNumberOfFrames;

        if (vkCreateDescriptorPool(m_pDevice->GetDevice(), &poolInfo, nullptr, &m_pDescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan descriptor pool!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateDescriptorSets()
    {
        std::array<VkDescriptorSetLayout, c_maxNumberOfFrames> layouts{};

        for (int index = 0; index < c_maxNumberOfFrames; ++index)
        {
            layouts[index] = m_pPipeline->GetFrameUniformDescriptorSetLayout();
        }

        VkDescriptorSetAllocateInfo allocInfo{};

        allocInfo.sType                 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool        = m_pDescriptorPool;
        allocInfo.descriptorSetCount    = c_maxNumberOfFrames;
        allocInfo.pSetLayouts           = layouts.data();

        std::array<VkDescriptorSet, c_maxNumberOfFrames> descriptorSets{};

        if (vkAllocateDescriptorSets(m_pDevice->GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate frame uniform descriptor sets!");
        }

        for (int index = 0; index < c_maxNumberOfFrames; ++index)
        {
            m_frames[index].frameDescriptorSet = descriptorSets[index];

            VkDescriptorBufferInfo bufferInfo{};

            bufferInfo.buffer   = m_frames[index].frameUniformedBuffer.GetBuffer();
            bufferInfo.offset   = 0;
            bufferInfo.range    = sizeof(sFrameUniformData);

            VkWriteDescriptorSet descriptorWrite{};

            descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrite.dstBinding      = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo     = &bufferInfo;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), 1, &descriptorWrite, 0, nullptr);
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateFrameUniformBuffer(sVulkanFrame& _rFrame, const cCamera& _rCamera)
        {
              sFrameUniformData frameData{};

        const float width  = static_cast<float>(m_pSwapchain->GetExtent().width);
        const float height = static_cast<float>(m_pSwapchain->GetExtent().height);

        const float aspectRatio = height > 0.0f ? width / height : 1.0f;

        _rCamera.GetViewMatrix(frameData.viewMatrix);
        _rCamera.GetProjectionMatrix(aspectRatio, frameData.projMatrix);
        _rCamera.GetViewProjectionMatrix(aspectRatio, frameData.viewProj);

        _rCamera.GetPosition(frameData.cameraPosition);
        _rCamera.GetDirection(frameData.cameraDirection);

        frameData.viewportSize[0] = width;
        frameData.viewportSize[1] = height;
        frameData.viewportSize[2] = width  > 0.0f ? 1.0f / width  : 0.0f;
        frameData.viewportSize[3] = height > 0.0f ? 1.0f / height : 0.0f;

        frameData.clipPlanes[0] = _rCamera.GetNearPlane();
        frameData.clipPlanes[1] = _rCamera.GetFarPlane();
        frameData.clipPlanes[2] = 0.0f;
        frameData.clipPlanes[3] = 0.0f;

        _rFrame.frameUniformedBuffer.Write(&frameData, sizeof(sFrameUniformData), 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------