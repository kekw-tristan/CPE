#include "vulkanRenderer.h"

#include "graphics/camera.h"
#include "graphics/frameUniformData.h"
#include "graphics/gfxConfig.h"
#include "graphics/instanceData.h"

#include "graphics/imgui/imguiManager.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanMesh.h"
#include "graphics/vulkan/vulkanPipeline.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanCommands.h"

#include "math/util.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
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

        CreateFrameResources();
        CreateMaterialBuffer();

        m_depthBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
        m_colorBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
        
        m_shadowMap.Create(*m_pDevice, 4096, 4096, 8);
        m_environment.Create(*m_pDevice, *m_pCommands);
        m_brdfLUT.Create(*m_pDevice, *m_pCommands);

        CreateDescriptorPool();
        CreateImGuiDescriptorPool();
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
        m_colorBuffer.ShutDown(*m_pDevice);

        m_shadowMap.Destroy(*m_pDevice);
        m_environment.Destroy(*m_pDevice);
        m_brdfLUT.Destroy(*m_pDevice);

        m_materialBuffer.Shutdown(*m_pDevice);
        m_materialStagingBuffer.Shutdown(*m_pDevice);

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

            rFrame.lightBuffer.Shutdown(*m_pDevice);
            rFrame.lightStagingBuffer.Shutdown(*m_pDevice);

            rFrame.shadowBuffer.Shutdown(*m_pDevice);
            rFrame.shadowStagingBuffer.Shutdown(*m_pDevice);
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

        if (m_pImGuiDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, m_pImGuiDescriptorPool, nullptr);
            m_pImGuiDescriptorPool = VK_NULL_HANDLE;
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

    void cVulkanRenderer::RecreateColorBuffer()
    {
        m_pDevice->WaitIdle();

        m_colorBuffer.ShutDown(*m_pDevice);

        m_colorBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
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

        Engine::GFX::ImGuiManager::BeginFrame();
        
        UpdateShadowBuffer(_rCamera);
        UpdateLightBuffer();
        UpdateMaterialBuffer();

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

        GFX::ImGuiManager::EndFrame(pCommandBuffer); 

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

    void cVulkanRenderer::DrawMeshIntances(cVulkanMesh* _pMesh, uint32_t _instanceCount, uint32_t _firstInstance)
    {
        if (m_renderPassType == sRenderPassType::None)
        {
            throw std::runtime_error("DrawMeshIntances() called outside of a render pass!");
        }

        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        VkBuffer vertexBuffers[]    = { _pMesh->GetVertexBuffer().GetBuffer() };
        VkDeviceSize offsets[]      = { 0 };

        vkCmdBindVertexBuffers(pCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(pCommandBuffer, _pMesh->GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(pCommandBuffer, _pMesh->GetIndexCount(), _instanceCount, 0, 0, _firstInstance);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateInstanceBuffer(std::vector<sInstanceData*>& _rInstances)
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

        VkDeviceSize instancesSize = sizeof(sInstanceData) * _rInstances.size(); 

        rFrame.instanceBufferStaging.Write(uploadData.data(), instancesSize);
        
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size      = instancesSize;

        vkCmdCopyBuffer(pCommandBuffer, rFrame.instanceBufferStaging.GetBuffer(), rFrame.instanceBuffer.GetBuffer(), 1, &copyRegion);

        VkBufferMemoryBarrier barrier{};

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = rFrame.instanceBuffer.GetBuffer();
        barrier.offset = 0;
        barrier.size = instancesSize;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            0,
            0, nullptr,
            1, &barrier,
            0, nullptr
        );

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateLightBuffer()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        const std::vector<sLight>& rLights = LightManager::GetLights();

        if (rLights.empty())
        {
            return;
        }

        std::vector<sLightGPU> gpuLights(rLights.size());

        for (size_t index = 0; index < rLights.size(); ++index)
        {
            const sLight& rLight = rLights[index];
            sLightGPU& rGPULight = gpuLights[index];

            rGPULight.positionRadius[0] = rLight.position.x();
            rGPULight.positionRadius[1] = rLight.position.y();
            rGPULight.positionRadius[2] = rLight.position.z();
            rGPULight.positionRadius[3] = rLight.radius;

            rGPULight.directionType[0] = rLight.direction.x();
            rGPULight.directionType[1] = rLight.direction.y();
            rGPULight.directionType[2] = rLight.direction.z();
            rGPULight.directionType[3] = static_cast<float>(rLight.type);

            rGPULight.colorIntensity[0] = rLight.color.x();
            rGPULight.colorIntensity[1] = rLight.color.y();
            rGPULight.colorIntensity[2] = rLight.color.z();
            rGPULight.colorIntensity[3] = rLight.intensity;

            rGPULight.spotData[0] = rLight.innerCone;
            rGPULight.spotData[1] = rLight.outerCone;
            rGPULight.spotData[2] = 0.0f;
            rGPULight.spotData[3] = 0.0f;

            rGPULight.shadowIndex   = index < m_lightShadowIndices.size() ? m_lightShadowIndices[index] : -1;
            rGPULight.padding0      = 0;
            rGPULight.padding1      = 0;
            rGPULight.padding2      = 0;
        }

        VkDeviceSize lightSize = sizeof(sLightGPU) * gpuLights.size();

        rFrame.lightStagingBuffer.Write(gpuLights.data(), lightSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = lightSize;

        vkCmdCopyBuffer(pCommandBuffer, rFrame.lightStagingBuffer.GetBuffer(), rFrame.lightBuffer.GetBuffer(), 1, &copyRegion);
    
        VkBufferMemoryBarrier barrier{};

        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = rFrame.lightBuffer.GetBuffer();
        barrier.offset              = 0;
        barrier.size                = lightSize;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            1, &barrier,
            0, nullptr
        );
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateMaterialBuffer()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        std::vector<sMaterial>& rMaterials = MaterialManager::GetMaterials();

        if (rMaterials.empty())
        {
            return;
        }

        VkDeviceSize materialSize = sizeof(sMaterial) * rMaterials.size();

        m_materialStagingBuffer.Write(rMaterials.data(), materialSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = materialSize;

        vkCmdCopyBuffer(pCommandBuffer, m_materialStagingBuffer.GetBuffer(), m_materialBuffer.GetBuffer(), 1, &copyRegion);
    
        VkBufferMemoryBarrier barrier{};

        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = m_materialBuffer.GetBuffer();
        barrier.offset              = 0;
        barrier.size                = materialSize;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            1, &barrier,
            0, nullptr
        );
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateShadowBuffer(const cCamera& _rCamera)
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;
        
        float cameraPosition[4];
        _rCamera.GetPosition(cameraPosition);

        const float width       = static_cast<float>(m_pSwapchain->GetExtent().width);
        const float height      = static_cast<float>(m_pSwapchain->GetExtent().height);
        const float aspectRatio = height > 0.0f ? width / height : 1.0f;

        const std::array<Math::cVec3f, 8> testCorners = Math::CalculateFrustumCorners(_rCamera, aspectRatio, 0.1f, 15.0f);

        std::vector<sLight>& rLights = LightManager::GetLights();

        m_shadowData.clear();
        m_shadowData.reserve(rLights.size());

        Math::cVec3f shadowCenter =
        {
            cameraPosition[0],
            cameraPosition[1],
            cameraPosition[2]
        };

        m_lightShadowIndices.assign(rLights.size(), -1);


        uint32_t nextLayer = 0;

        for (uint32_t lightIndex = 0; lightIndex < static_cast<uint32_t>(rLights.size()); ++lightIndex)
        {
            sLight&  rLight          = rLights[lightIndex];
            uint32_t requiredLayers  = 0;

            if (!rLight.castsShadow)
            {
                continue;
            }

            sShadowDataGPU shadow{};

            shadow.lightIndex   = lightIndex;
            shadow.firstLayer   = nextLayer;
            shadow.matrixCount  = 0;
            shadow.padding      = 0;

            switch (rLight.type)
            {
                case sLightType::Directional:
                {
                    Math::cVec3f direction = rLight.direction;

                    const float directionLength = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y() + direction.z() * direction.z());

                    if (directionLength <= 0.0001f)
                    {
                        continue;
                    }

                    direction =
                    {
                        direction.x() / directionLength,
                        direction.y() / directionLength,
                        direction.z() / directionLength
                    };

                    const VkExtent2D extent = m_pSwapchain->GetExtent();

                    const float width       = static_cast<float>(extent.width);
                    const float height      = static_cast<float>(extent.height);
                    const float aspectRatio = height > 0.f ? width / height : 1.f;

                    const float cameraNear = _rCamera.GetNearPlane();

                    for (uint32_t cascadeIndex = 0; cascadeIndex < c_directionalCascadeCount; ++cascadeIndex)
                    {
                        const float cascadeNear = cascadeIndex == 0 ? cameraNear : c_directionalCascadeSplits[cascadeIndex - 1];
                        const float cascadeFar  = c_directionalCascadeSplits[cascadeIndex];

                        const std::array<Math::cVec3f, 8> corners = Math::CalculateFrustumCorners(_rCamera, aspectRatio, cascadeNear, cascadeFar);

                        shadow.viewProjection[cascadeIndex] = CalculateDirectionalShadowMatrix(corners, direction, c_shadowMapResolution);
                        shadow.cascadeSplits[cascadeIndex]  = cascadeFar;
                    }

                    shadow.matrixCount = c_directionalCascadeCount;
                    requiredLayers = c_directionalCascadeCount;

                    break;
                }

                case sLightType::Spot:
                {
                    Math::cVec3f direction = rLight.direction;

                    const float directionLength = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y() + direction.z() * direction.z());

                    if (directionLength <= 0.0001f)
                    {
                        continue;
                    }

                    direction =
                    {
                        direction.x() / directionLength,
                        direction.y() / directionLength,
                        direction.z() / directionLength
                    };

                    const Math::cVec3f lightTarget =
                    {
                        rLight.position.x() + direction.x(),
                        rLight.position.y() + direction.y(),
                        rLight.position.z() + direction.z()
                    };

                    Math::cVec3f up = { 0.f, 1.f, 0.f };

                    if (std::abs(direction.y()) > 0.99f)
                    {
                        up = { 0.f, 0.f, 1.f };
                    }

                    const float outerCone = std::clamp(rLight.outerCone, -1.f, 1.f);
                    const float outerAngle = std::acos(outerCone);
                    const float fieldOfView = outerAngle * 2.f;

                    const float nearPlane = 0.001f;
                    const float farPlane = std::max(rLight.radius, nearPlane + 0.01f);

                    const Math::cMatrix4x4f lightView = Math::cMatrix4x4f::lookAtRH(rLight.position, lightTarget, up);
                    const Math::cMatrix4x4f lightProjection = Math::cMatrix4x4f::perspectiveRH(fieldOfView, 1.f, nearPlane, farPlane);
                    const Math::cMatrix4x4f lightViewProjection = lightView * lightProjection;

                    shadow.viewProjection[0] = lightViewProjection;

                    shadow.matrixCount = 1;

                    requiredLayers = 1;
                    break;
                }

                case sLightType::Point:
                {
                    const float nearPlane = 0.00001f;
                    const float farPlane = std::max(rLight.radius, nearPlane + 0.01f);

                    constexpr float c_pi = 3.14159265358979323846f;

                    const Math::cMatrix4x4f lightProjection = Math::cMatrix4x4f::perspectiveRH(c_pi * 0.5f, 1.f, nearPlane, farPlane);

                    const Math::cVec3f directions[6] =
                    {
                        {  1.f,  0.f,  0.f },
                        { -1.f,  0.f,  0.f },
                        {  0.f,  1.f,  0.f },
                        {  0.f, -1.f,  0.f },
                        {  0.f,  0.f,  1.f },
                        {  0.f,  0.f, -1.f }
                    };

                    const Math::cVec3f upVectors[6] =
                    {
                        { 0.f, -1.f,  0.f },
                        { 0.f, -1.f,  0.f },
                        { 0.f,  0.f,  1.f },
                        { 0.f,  0.f, -1.f },
                        { 0.f, -1.f,  0.f },
                        { 0.f, -1.f,  0.f }
                    };

                    for (uint32_t face = 0; face < 6; ++face)
                    {
                        const Math::cVec3f target =
                        {
                            rLight.position.x() + directions[face].x(),
                            rLight.position.y() + directions[face].y(),
                            rLight.position.z() + directions[face].z()
                        };

                        const Math::cMatrix4x4f lightView = Math::cMatrix4x4f::lookAtRH(rLight.position, target, upVectors[face]);

                        shadow.viewProjection[face] = lightView * lightProjection;
                    }

                    shadow.matrixCount = 6;

                    requiredLayers = 6;
                    break;
                }

                default:
                {
                    continue;
                }
            }

            if (nextLayer + requiredLayers > m_shadowMap.GetLayerCount())
            {
                continue;
            }

            shadow.firstLayer = nextLayer;

            const int32_t shadowIndex = static_cast<int32_t>(m_shadowData.size());

            m_lightShadowIndices[lightIndex] = shadowIndex;

            m_shadowData.push_back(shadow);

            nextLayer += requiredLayers;
        }

        if (m_shadowData.empty())
        {
            return;
        }

        const VkDeviceSize shadowDataSize = sizeof(sShadowDataGPU) * m_shadowData.size();

        rFrame.shadowStagingBuffer.Write(m_shadowData.data(), shadowDataSize);

        VkBufferCopy copyRegion{};

        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = shadowDataSize;

        vkCmdCopyBuffer(pCommandBuffer, rFrame.shadowStagingBuffer.GetBuffer(), rFrame.shadowBuffer.GetBuffer(), 1, &copyRegion);

        VkBufferMemoryBarrier barrier{};

        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = rFrame.shadowBuffer.GetBuffer();
        barrier.offset              = 0;
        barrier.size                = shadowDataSize;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            1, &barrier,
            0, nullptr
        );
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginShadowRendering()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        m_shadowMap.GetImageResource().TransitionLayout(
            *m_pDevice,
            pCommandBuffer,
            m_shadowMapLayout,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );

        m_shadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndShadowRendering()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        m_shadowMap.GetImageResource().TransitionLayout(
            *m_pDevice,
            pCommandBuffer,
            m_shadowMapLayout,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );

        m_shadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginShadowDraw(uint32_t _shadowIndex, uint32_t _matrixIndex)
    {
        if (_shadowIndex >= m_shadowData.size())
        {
            throw std::runtime_error("Invalid shadow index!");
        }

        const sShadowDataGPU& shadow = m_shadowData[_shadowIndex];

        if (_matrixIndex >= shadow.matrixCount)
        {
            throw std::runtime_error("Invalid shadow matrix index!");
        }

        const uint32_t layer = shadow.firstLayer + _matrixIndex;

        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        m_renderPassType = sRenderPassType::Shadow;

        VkRenderingAttachmentInfo depthAttachment{};

        depthAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView               = m_shadowMap.GetLayerImageView(layer);
        depthAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo renderingInfo{};

        renderingInfo.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset     = { 0, 0 };
        renderingInfo.renderArea.extent     = { m_shadowMap.GetWidth(), m_shadowMap.GetHeight() };
        renderingInfo.layerCount            = 1;
        renderingInfo.colorAttachmentCount  = 0;
        renderingInfo.pColorAttachments     = nullptr;
        renderingInfo.pDepthAttachment      = &depthAttachment;
        renderingInfo.pStencilAttachment    = nullptr;

        vkCmdBeginRendering(pCommandBuffer, &renderingInfo);

        VkViewport viewport{};

        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = static_cast<float>(m_shadowMap.GetWidth());
        viewport.height     = static_cast<float>(m_shadowMap.GetHeight());
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;

        vkCmdSetViewport(pCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};

        scissor.offset = { 0, 0 };
        scissor.extent = { m_shadowMap.GetWidth(), m_shadowMap.GetHeight() };

        vkCmdSetScissor(pCommandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetShadowPipeline());

        vkCmdBindDescriptorSets(
            pCommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pPipeline->GetShadowPipelineLayout(),
            0,
            1,
            &rFrame.frameDescriptorSet,
            0,
            nullptr
        );

        sShadowPushConstants pushConstants{};

        pushConstants.shadowIndex = _shadowIndex;
        pushConstants.matrixIndex = _matrixIndex;

        vkCmdPushConstants(
            pCommandBuffer,
            m_pPipeline->GetShadowPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(sShadowPushConstants),
            &pushConstants
        );
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawShadowMeshInstances(cVulkanMesh* _pMesh, uint32_t _instanceCount, uint32_t _firstInstance)
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        VkBuffer vertexBuffers[]    = { _pMesh->GetVertexBuffer().GetBuffer() };
        VkDeviceSize offsets[]      = { 0 };

        vkCmdBindVertexBuffers(pCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(pCommandBuffer, _pMesh->GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(pCommandBuffer, _pMesh->GetIndexCount(), _instanceCount, 0, 0, _firstInstance);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndShadowDraw()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        vkCmdEndRendering(pCommandBuffer);

        m_renderPassType = sRenderPassType::None;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginDraw()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        m_renderPassType = sRenderPassType::Main;

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

        colorAttachment.sType               = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView           = m_colorBuffer.GetImageView();
        colorAttachment.imageLayout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp              = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp             = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue          = clearValue;
        colorAttachment.resolveMode         = VK_RESOLVE_MODE_AVERAGE_BIT;
        colorAttachment.resolveImageView    = swapchainImageView;
        colorAttachment.resolveImageLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

    VkDescriptorPool cVulkanRenderer::GetImguiDescriptorPool()
    {
        return m_pImGuiDescriptorPool;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanRenderer::GetShadowCount() const
    {
        return static_cast<uint32_t>(m_shadowData.size());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cVulkanRenderer::GetShadowMatrixCount(uint32_t _shadowIndex) const
    {
        return m_shadowData[_shadowIndex].matrixCount;
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


            // frame data
            rFrame.frameUniformedBuffer.Create(*m_pDevice, sizeof(sFrameUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.frameUniformedBuffer.Map(*m_pDevice, sizeof(sFrameUniformData), 0);

            // instances
            rFrame.instanceBuffer.Create(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.instanceBufferStaging.Create(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.instanceBufferStaging.Map(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, 0);

            // light
            rFrame.lightBuffer.Create(*m_pDevice, sizeof(sLightGPU) * c_maxNumberOfLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.lightStagingBuffer.Create(*m_pDevice, sizeof(sLightGPU) * c_maxNumberOfLights, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.lightStagingBuffer.Map(*m_pDevice, sizeof(sLightGPU) * c_maxNumberOfLights, 0);
            
            // shadows
            rFrame.shadowBuffer.Create(*m_pDevice, sizeof(sShadowDataGPU) * c_maxNumberOfLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.shadowStagingBuffer.Create(*m_pDevice, sizeof(sShadowDataGPU) * c_maxNumberOfLights, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.shadowStagingBuffer.Map(*m_pDevice, sizeof(sShadowDataGPU) * c_maxNumberOfLights, 0);
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

        std::array<VkDescriptorPoolSize, 4> poolSizes{};

        // frame uniform buffer
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = c_maxNumberOfFrames;

        // instance, light, material, shadow storage buffer 
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = c_maxNumberOfFrames * 4;

        // shadow image + environment image + BRDF LUT + irradiance image
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[2].descriptorCount = c_maxNumberOfFrames * 4;

        // shadow sampler + environment sampler + BRDF sampler
        poolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[3].descriptorCount = c_maxNumberOfFrames * 3;

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

    void cVulkanRenderer::CreateImGuiDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 11> poolSizes =
        {
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        if (vkCreateDescriptorPool(
                m_pDevice->GetDevice(),
                &poolInfo,
                nullptr,
                &m_pImGuiDescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create ImGui descriptor pool.");
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

            VkDescriptorBufferInfo lightBufferInfo{};

            lightBufferInfo.buffer  = m_frames[index].lightBuffer.GetBuffer();
            lightBufferInfo.offset  = 0;
            lightBufferInfo.range   = sizeof(sLightGPU) * c_maxNumberOfLights;

            VkDescriptorBufferInfo materialBufferInfo{};

            materialBufferInfo.buffer   = m_materialBuffer.GetBuffer();
            materialBufferInfo.offset   = 0;
            materialBufferInfo.range    = sizeof(sMaterial) * c_maxNumberOfMaterials;

            VkDescriptorBufferInfo shadowBufferInfo{};

            shadowBufferInfo.buffer = m_frames[index].shadowBuffer.GetBuffer();
            shadowBufferInfo.offset = 0;
            shadowBufferInfo.range  = sizeof(sShadowDataGPU) * c_maxNumberOfLights;

            VkDescriptorImageInfo shadowImageInfo{};

            shadowImageInfo.sampler     = m_shadowMap.GetSampler();
            shadowImageInfo.imageView   = m_shadowMap.GetImageView();
            shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo shadowSamplerInfo{};

            shadowSamplerInfo.sampler       = m_shadowMap.GetSampler();
            shadowSamplerInfo.imageView     = VK_NULL_HANDLE;
            shadowSamplerInfo.imageLayout   = VK_IMAGE_LAYOUT_UNDEFINED;

            VkDescriptorImageInfo environmentImageInfo{};

            environmentImageInfo.sampler     = VK_NULL_HANDLE;
            environmentImageInfo.imageView   = m_environment.GetImageView();
            environmentImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo environmentSamplerInfo{};

            environmentSamplerInfo.sampler     = m_environment.GetSampler();
            environmentSamplerInfo.imageView   = VK_NULL_HANDLE;
            environmentSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkDescriptorImageInfo brdfImageInfo{};

            brdfImageInfo.sampler       = VK_NULL_HANDLE;
            brdfImageInfo.imageView     = m_brdfLUT.GetImageView();
            brdfImageInfo.imageLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo brdfSamplerInfo{};

            brdfSamplerInfo.sampler     = m_brdfLUT.GetSampler();
            brdfSamplerInfo.imageView   = VK_NULL_HANDLE;
            brdfSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkDescriptorImageInfo irradianceImageInfo{};

            irradianceImageInfo.sampler     = VK_NULL_HANDLE;
            irradianceImageInfo.imageView   = m_environment.GetIrradianceImageView();
            irradianceImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            std::array<VkWriteDescriptorSet, 12> descriptorWrites{};

            descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[0].dstBinding      = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo     = &frameBufferInfo;

            descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[1].dstBinding      = 1;
            descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo     = &instanceBufferInfo;

            descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[2].dstBinding      = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo     = &lightBufferInfo;

            descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[3].dstBinding      = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo     = &materialBufferInfo;

            descriptorWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[4].dstBinding      = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pBufferInfo     = &shadowBufferInfo;

            descriptorWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[5].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[5].dstBinding      = 5;
            descriptorWrites[5].dstArrayElement = 0;
            descriptorWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[5].descriptorCount = 1;
            descriptorWrites[5].pImageInfo      = &shadowImageInfo;

            descriptorWrites[6].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[6].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[6].dstBinding      = 6;
            descriptorWrites[6].dstArrayElement = 0;
            descriptorWrites[6].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrites[6].descriptorCount = 1;
            descriptorWrites[6].pImageInfo      = &shadowSamplerInfo;

            descriptorWrites[7].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[7].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[7].dstBinding      = 7;
            descriptorWrites[7].dstArrayElement = 0;
            descriptorWrites[7].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[7].descriptorCount = 1;
            descriptorWrites[7].pImageInfo      = &environmentImageInfo;

            descriptorWrites[8].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[8].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[8].dstBinding      = 8;
            descriptorWrites[8].dstArrayElement = 0;
            descriptorWrites[8].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrites[8].descriptorCount = 1;
            descriptorWrites[8].pImageInfo      = &environmentSamplerInfo;

            descriptorWrites[9].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[9].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[9].dstBinding      = 9;
            descriptorWrites[9].dstArrayElement = 0;
            descriptorWrites[9].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[9].descriptorCount = 1;
            descriptorWrites[9].pImageInfo      = &brdfImageInfo;

            descriptorWrites[10].sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[10].dstSet             = m_frames[index].frameDescriptorSet;
            descriptorWrites[10].dstBinding         = 10;
            descriptorWrites[10].dstArrayElement    = 0;
            descriptorWrites[10].descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrites[10].descriptorCount    = 1;
            descriptorWrites[10].pImageInfo         = &brdfSamplerInfo;

            descriptorWrites[11].sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[11].dstSet             = m_frames[index].frameDescriptorSet;
            descriptorWrites[11].dstBinding         = 11;
            descriptorWrites[11].dstArrayElement    = 0;
            descriptorWrites[11].descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[11].descriptorCount    = 1;
            descriptorWrites[11].pImageInfo         = &irradianceImageInfo;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateMaterialBuffer()
    {
        
        m_materialBuffer.Create(*m_pDevice, sizeof(sMaterial) * c_maxNumberOfMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_materialStagingBuffer.Create(*m_pDevice, sizeof(sMaterial) * c_maxNumberOfMaterials, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_materialStagingBuffer.Map(*m_pDevice, sizeof(sMaterial) * c_maxNumberOfMaterials, 0);
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

        frameData.lightCount    = static_cast<uint32_t>(LightManager::GetLights().size());
        frameData.materialCount = static_cast<uint32_t>(MaterialManager::GetMaterials().size());

        _rFrame.frameUniformedBuffer.Write(&frameData, sizeof(sFrameUniformData), 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------