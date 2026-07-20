#include "vulkanRenderer.h"

#include "graphics/camera.h"
#include "graphics/frameUniformData.h"
#include "graphics/gfxConfig.h"
#include "graphics/instanceData.h"
#include "graphics/pushConstants.h"

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

    void cVulkanRenderer::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanPipeline& _rPipeline)
    {
        m_pDevice    = &_rDevice;
        m_pSwapchain = &_rSwapChain;
        m_pCommands  = &_rCommands;
        m_pPipeline  = &_rPipeline;

        m_currentFrame = 0;
        m_hasFrameStarted = false; 

        m_depthBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);

        CreateFrameResources();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateRenderFinishedSemaphores();
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

            rFrame.instanceBuffer.Shutdown(*m_pDevice);
            rFrame.instanceBufferStaging.Shutdown(*m_pDevice);
        }

        for (VkSemaphore sem : m_renderFinishedSemaphores)
        {
            if (sem != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_pDevice->GetDevice(), sem, nullptr);
            }
        }
        
        m_renderFinishedSemaphores.clear();

        if (m_pDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, m_pDescriptorPool, nullptr);
            m_pDescriptorPool = VK_NULL_HANDLE;
        }

        m_currentFrame = 0;

        m_pPipeline  = nullptr;
        m_pCommands  = nullptr;
        m_pSwapchain = nullptr;
        m_pDevice    = nullptr;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::RecreateDepthBuffer()
    {
        m_pDevice->WaitIdle();

        m_depthBuffer.ShutDown(*m_pDevice);

        m_depthBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::SubmitMesh(const cVulkanMesh&_rMesh)
    {
        if (!_rMesh.IsValid())
        {
            return;
        }

        m_submittedMeshes.push_back(&_rMesh);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::ClearSubmittedMeshes()
    {
        m_submittedMeshes.clear();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanRenderer::BeginFrame(const cCamera &_rCamera)
    {
        if (m_hasFrameStarted)
        {
            throw std::runtime_error("BeginFrame() called while frame is already started!");
        }

        VkDevice        device          = m_pDevice->GetDevice();
        sVulkanFrame&   frame           = m_frames[m_currentFrame];
        
        m_imageIndex = 0;

        vkWaitForFences(device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX); 
        
         
        VkResult acquireResult = vkAcquireNextImageKHR(
            device, 
            m_pSwapchain->GetSwapchain(),
            UINT64_MAX,frame.imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &m_imageIndex
        );

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return false;
        }

         m_hasFrameStarted = true;

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        if (m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(device, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
        }

        m_imagesInFlight[m_imageIndex] = frame.inFlightFence;

        UpdateFrameUniformBuffer(frame, _rCamera);

        VkCommandBuffer commandBuffer = frame.pCommandBuffer; 

        vkResetCommandBuffer(commandBuffer, 0); 

        VkCommandBufferBeginInfo beginInfo{}; 
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo)!= VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanRenderer::EndFrame()
    {
        if (!m_hasFrameStarted)
        {
            throw std::runtime_error("EndFrame() called without BeginFrame()!");
        }

        VkDevice device = m_pDevice->GetDevice();
        sVulkanFrame& rFrame = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer = rFrame.pCommandBuffer;

        EndDraw(pCommandBuffer, m_imageIndex);

        if (vkEndCommandBuffer(pCommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to end command buffer!");
        }

        vkResetFences(device, 1, &rFrame.inFlightFence);

        VkSemaphore          waitSemaphores[]   = { rFrame.imageAvailableSemaphore };
        VkSemaphore          signalSemaphores[] = { m_renderFinishedSemaphores[m_imageIndex] };
        VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};

        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &pCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, rFrame.inFlightFence);

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
        presentInfo.pImageIndices       = &m_imageIndex;

        m_hasFrameStarted   = false; 
        m_hasFrameStarted   = false;
        m_currentFrame      = (m_currentFrame + 1) % c_maxNumberOfFrames;

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

    void cVulkanRenderer::DrawMeshIntances(cVulkanMesh* _pMesh, std::vector<sInstanceData*>& _rInstances)
    {
        sVulkanFrame& rFrame            = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;
        
        VkBuffer vertexBuffers[]    = { _pMesh->GetVertexBuffer().GetBuffer() };
        VkDeviceSize offsets[]      = { 0 };

        vkCmdBindVertexBuffers(pCommandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(pCommandBuffer, _pMesh->GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPipelineLayout(), 0, 1, &rFrame.frameDescriptorSet, 0, nullptr);

        vkCmdDrawIndexed(pCommandBuffer, _pMesh->GetIndexCount(), static_cast<uint32_t>(_rInstances.size()), 0, 0, 0);

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateInstanceBuffer(std::vector<sInstanceData *>& _rInstances)
    {
        sVulkanFrame& rFrame            = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        // upload instances 

        std::vector<sInstanceData> uploadData;
        uploadData.reserve(_rInstances.size());

        for (sInstanceData* instance : _rInstances)
        {
            uploadData.push_back(*instance);
        }

        VkDeviceSize numberOfInstances = sizeof(sInstanceData) * _rInstances.size(); 

        rFrame.instanceBufferStaging.Write(uploadData.data(), numberOfInstances);
        
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size      = numberOfInstances;

        vkCmdCopyBuffer(pCommandBuffer, rFrame.instanceBufferStaging.GetBuffer(), rFrame.instanceBuffer.GetBuffer(), 1, &copyRegion);


    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginDraw()
    {
        sVulkanFrame& rFrame            = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        VkImage     swapchainImage     = m_pSwapchain->GetImages()[m_imageIndex];
        VkImageView swapchainImageView = m_pSwapchain->GetImageViews()[m_imageIndex];
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
            pCommandBuffer,
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

        vkCmdBeginRendering(pCommandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = static_cast<float>(extent.width);
        viewport.height     = static_cast<float>(extent.height);
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;

        vkCmdSetViewport(pCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        vkCmdSetScissor(pCommandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(
            pCommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pPipeline->GetPipeline()
        );

        vkCmdBindDescriptorSets(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPipelineLayout(), 0, 1, &rFrame.frameDescriptorSet, 0, nullptr);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex)
    {
        vkCmdEndRendering(_pCommandBuffer);

        VkImage swapchainImage = m_pSwapchain->GetImages()[_imageIndex];

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
            _pCommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierToPresent
        );
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
        
            if (vkCreateFence(device, &fenceInfo, nullptr, &rFrame.inFlightFence) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create in-flight fence!");
            }

            VkCommandBufferAllocateInfo commandBufferAllocInfo{};

            commandBufferAllocInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocInfo.commandPool          = m_pCommands->GetCommandPool();
            commandBufferAllocInfo.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocInfo.commandBufferCount   = 1;

            if (vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &rFrame.pCommandBuffer) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate frame command buffer!");
            }


            rFrame.frameUniformedBuffer.Create(*m_pDevice, sizeof(sFrameUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.frameUniformedBuffer.Map(*m_pDevice, sizeof(sFrameUniformData), 0);

            rFrame.instanceBuffer.Create(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.instanceBufferStaging.Create(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.instanceBufferStaging.Map(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, 0);


        }

        std::cout << "Vulkan sync objects created." << std::endl;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateRenderFinishedSemaphores()
    {
        uint32_t imageCount = m_pSwapchain->GetImageCount(); 
        VkDevice device = m_pDevice->GetDevice();

        m_renderFinishedSemaphores.resize(imageCount);
        m_imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

        for (uint32_t index = 0; index < imageCount; ++index)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[index]);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateDescriptorPool()
    {

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        // Binding 0 - Frame Uniform Buffer
        poolSizes[0].type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount    = c_maxNumberOfFrames;


        // Binding 1 - Instance Storage Buffer
        poolSizes[1].type               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount    = c_maxNumberOfFrames;

        VkDescriptorPoolCreateInfo poolInfo{};

        poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes     = poolSizes.data();
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

            VkDescriptorBufferInfo frameBufferInfo{};

            frameBufferInfo.buffer   = m_frames[index].frameUniformedBuffer.GetBuffer();
            frameBufferInfo.offset   = 0;
            frameBufferInfo.range    = sizeof(sFrameUniformData);

            VkDescriptorBufferInfo instanceBufferInfo{};

            instanceBufferInfo.buffer   = m_frames[index].instanceBuffer.GetBuffer();
            instanceBufferInfo.offset   = 0;
            instanceBufferInfo.range    = sizeof(sInstanceData) * c_maxNumberOfInstances;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[0].dstBinding      = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo     = &frameBufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_frames[index].frameDescriptorSet;
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &instanceBufferInfo;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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