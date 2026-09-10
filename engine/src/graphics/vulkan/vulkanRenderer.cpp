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

#include "graphics/vulkan/reflectionProbe.h"
#include "graphics/vulkan/reflectionProbePushConstants.h"
#include "graphics/vulkan/reflectionProbePrefilterPushConstants.h"

#include "math/util.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::SetBackgroundColor(const std::array<float, 4>& _rColor)
    {
        m_backgroundColor = _rColor;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanPipeline& _rPipeline, const sEnvironmentSettings& _rEnvironment)
    {
        m_pDevice = &_rDevice;
        m_pSwapchain = &_rSwapChain;
        m_pCommands = &_rCommands;
        m_pPipeline = &_rPipeline;

        m_currentFrame = 0;
        m_hasFrameStarted = false;
        m_renderPassType = sRenderPassType::None;

        m_activeLightIndices.reserve(c_maxNumberOfActiveLights);
        m_previousActiveLightIndices.reserve(c_maxNumberOfActiveLights);
        m_activeLightCandidates.reserve(c_maxNumberOfLights);

        // -------------------------------------------------------------------------------------------------------------------------
        // Frame resources
        // -------------------------------------------------------------------------------------------------------------------------

        CreateFrameResources();
        CreateMaterialBuffer();

        // -------------------------------------------------------------------------------------------------------------------------
        // Main rendering buffers
        // -------------------------------------------------------------------------------------------------------------------------

        m_depthBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
        m_colorBuffer.Init(*m_pDevice, *m_pSwapchain, *m_pCommands);
        CreateBloomBuffer();
        CreatePostProcessSampler();
        CreateAmbientOcclusionBuffers();

        // -------------------------------------------------------------------------------------------------------------------------
        // Shadows
        // -------------------------------------------------------------------------------------------------------------------------

        m_shadowMap.Create(*m_pDevice, c_shadowMapResolution, c_shadowMapResolution, 8);

        // -------------------------------------------------------------------------------------------------------------------------
        // Global IBL
        // -------------------------------------------------------------------------------------------------------------------------

        m_environment.Create(*m_pDevice, *m_pCommands, _rEnvironment);
        m_brdfLUT.Create(*m_pDevice, *m_pCommands);

        // -------------------------------------------------------------------------------------------------------------------------
        // Reflection probes
        // -------------------------------------------------------------------------------------------------------------------------

        ReflectionProbeManager::Clear();
        ReflectionProbeManager::SetCellSize(32.0f);

        //constexpr uint32_t probeCountX = 5;
        //constexpr uint32_t probeCountZ = 2;
        //
        //constexpr float spacingX = 16.0f;
        //constexpr float spacingZ = 16.0f;
        //
        //constexpr float halfSizeX = 12.0f;
        //constexpr float halfSizeZ = 12.0f;
        //
        //constexpr float startX = -32.0f;
        //constexpr float startZ = -8.0f;
        //
        //for (uint32_t z = 0; z < probeCountZ; ++z)
        //{
        //    for (uint32_t x = 0; x < probeCountX; ++x)
        //    {
        //        const float positionX = startX + static_cast<float>(x) * spacingX;
        //        const float positionZ = startZ + static_cast<float>(z) * spacingZ;
        //
        //        sReflectionProbe probe{};
        //
        //        probe.position      = { positionX, 3.0f, positionZ };
        //        probe.boxMin        = { positionX - halfSizeX, -5.0f, positionZ - halfSizeZ };
        //        probe.boxMax        = { positionX + halfSizeX, 12.0f, positionZ + halfSizeZ };
        //        probe.radius        = 30.0f;
        //        probe.blendDistance = 4.0f;
        //        probe.resolution    = 256;
        //        probe.dirty         = true;
        //
        //        ReflectionProbeManager::AddProbe(probe);
        //    }
        //}

        const uint32_t reflectionProbeCount = ReflectionProbeManager::GetProbeCount();

        m_activeReflectionProbeIndex = UINT32_MAX;
        m_activeReflectionProbeHandles.reserve(c_maxNumberOfActiveReflectionProbes);
        m_visibleReflectionProbeHandles.reserve(c_maxNumberOfActiveReflectionProbes);

        m_vulkanReflectionProbes.clear();
        m_vulkanReflectionProbes.reserve(reflectionProbeCount);

        m_reflectionProbeCaptureLayouts.assign(reflectionProbeCount, VK_IMAGE_LAYOUT_UNDEFINED);
        m_reflectionProbePrefilteredLayouts.assign(reflectionProbeCount, VK_IMAGE_LAYOUT_UNDEFINED);
        m_reflectionProbePrefilterDescriptorSets.assign(reflectionProbeCount, VK_NULL_HANDLE);

        uint32_t maximumReflectionProbeResolution = 1;

        for (ReflectionProbeHandle probeHandle = 0; probeHandle < reflectionProbeCount; ++probeHandle)
        {
            const sReflectionProbe& rProbe = ReflectionProbeManager::GetProbe(probeHandle);

            std::unique_ptr<cVulkanReflectionProbe> pVulkanProbe = std::make_unique<cVulkanReflectionProbe>();

            pVulkanProbe->Create(*m_pDevice, rProbe.resolution);

            maximumReflectionProbeResolution = std::max(maximumReflectionProbeResolution, rProbe.resolution);

            m_vulkanReflectionProbes.push_back(std::move(pVulkanProbe));
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Shared reflection probe depth buffer
        //
        // Probes are captured sequentially, therefore all probes can share one depth image.
        // Use the largest configured probe resolution so probes with different resolutions are possible later.
        // -------------------------------------------------------------------------------------------------------------------------

        m_reflectionProbeDepthImage.Create(
            *m_pDevice,
            maximumReflectionProbeResolution,
            maximumReflectionProbeResolution,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_SAMPLE_COUNT_1_BIT
        );

        m_reflectionProbeDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_reflectionProbeDepthResolution = maximumReflectionProbeResolution;

        // -------------------------------------------------------------------------------------------------------------------------
        // Descriptor resources
        //
        // Reflection probe images must already exist before CreateDescriptorSets(), because binding 12 references their
        // prefiltered cube image views.
        // -------------------------------------------------------------------------------------------------------------------------

        CreateDescriptorPool();
        CreateImGuiDescriptorPool();

        CreateDescriptorSets();
        CreatePostProcessDescriptorSet();
        CreateReflectionProbePrefilterDescriptorSets();

        // -------------------------------------------------------------------------------------------------------------------------
        // Synchronization
        // -------------------------------------------------------------------------------------------------------------------------

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
        DestroyBloomBuffer();

        DestroyAmbientOcclusionBuffers();

        if (m_postProcessSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_postProcessSampler, nullptr);
            m_postProcessSampler = VK_NULL_HANDLE;
        }

        m_shadowMap.Destroy(*m_pDevice);
        m_environment.Destroy(*m_pDevice);
        m_brdfLUT.Destroy(*m_pDevice);

        m_reflectionProbeDepthImage.Destroy(*m_pDevice);

        for (std::unique_ptr<cVulkanReflectionProbe>& pReflectionProbe : m_vulkanReflectionProbes)
        {
            if (pReflectionProbe)
            {
                pReflectionProbe->Destroy(*m_pDevice);
            }
        }

        m_vulkanReflectionProbes.clear();

        m_reflectionProbeCaptureLayouts.clear();
        m_reflectionProbePrefilteredLayouts.clear();
        m_reflectionProbePrefilterDescriptorSets.clear();

        m_materialBuffer.Shutdown(*m_pDevice);
        for (cVulkanBuffer& rBuffer : m_materialStagingBuffers)
        {
            rBuffer.Shutdown(*m_pDevice);
        }

        for (sVulkanFrame& rFrame : m_frames)
        {
            if (rFrame.timestampQueryPool != VK_NULL_HANDLE)
            {
                vkDestroyQueryPool(device, rFrame.timestampQueryPool, nullptr);
                rFrame.timestampQueryPool = VK_NULL_HANDLE;
                rFrame.timestampsSubmitted = false;
            }

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

            rFrame.particleBuffer.Shutdown(*m_pDevice);
            rFrame.particleCount = 0;
            rFrame.healthBarBuffer.Shutdown(*m_pDevice);
            rFrame.healthBarCount = 0;

            rFrame.instanceBuffer.Shutdown(*m_pDevice);
            rFrame.instanceBufferStaging.Shutdown(*m_pDevice);

            rFrame.lightBuffer.Shutdown(*m_pDevice);
            rFrame.lightStagingBuffer.Shutdown(*m_pDevice);

            rFrame.activeLightIndexBuffer.Shutdown(*m_pDevice);
            rFrame.activeLightIndexStagingBuffer.Shutdown(*m_pDevice);

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
        DestroyBloomBuffer();
        CreateBloomBuffer();
        DestroyAmbientOcclusionBuffers();
        CreateAmbientOcclusionBuffers();
        UpdatePostProcessDescriptorSet();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateAmbientOcclusionBuffers()
    {
        const VkExtent2D extent = m_pSwapchain->GetExtent();
        const uint32_t halfWidth = std::max(1u, (extent.width + 1) / 2);
        const uint32_t halfHeight = std::max(1u, (extent.height + 1) / 2);
        const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        m_occlusionGeometry.Create(*m_pDevice, extent.width, extent.height, VK_FORMAT_R16G16B16A16_SFLOAT,
            usage, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);
        m_occlusionRaw.Create(*m_pDevice, halfWidth, halfHeight, VK_FORMAT_R16G16B16A16_SFLOAT,
            usage, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);
        m_occlusionFiltered.Create(*m_pDevice, halfWidth, halfHeight, VK_FORMAT_R16G16B16A16_SFLOAT,
            usage, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);
        m_occlusionDepth.Create(*m_pDevice, extent.width, extent.height, VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_SAMPLE_COUNT_1_BIT);

        VkCommandBuffer commandBuffer = m_pCommands->BeginSingleTimeCommands(*m_pDevice);
        for (cVulkanImage* pImage : { &m_occlusionGeometry, &m_occlusionRaw, &m_occlusionFiltered })
        {
            pImage->TransitionLayout(*m_pDevice, commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        m_occlusionDepth.TransitionLayout(*m_pDevice, commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_pCommands->EndSingleTimeCommands(*m_pDevice, commandBuffer);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DestroyAmbientOcclusionBuffers()
    {
        m_occlusionGeometry.Destroy(*m_pDevice);
        m_occlusionDepth.Destroy(*m_pDevice);
        m_occlusionRaw.Destroy(*m_pDevice);
        m_occlusionFiltered.Destroy(*m_pDevice);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateAmbientOcclusionDescriptors()
    {
        const std::array<VkImageView, 3> views =
        {
            m_occlusionGeometry.GetImageView(), m_occlusionRaw.GetImageView(), m_occlusionFiltered.GetImageView()
        };

        // Called only during initialization or resize, with no frames in flight.
        for (sVulkanFrame& rFrame : m_frames)
        {
            std::array<VkDescriptorImageInfo, 3> images{};
            std::array<VkWriteDescriptorSet, 3> writes{};
            for (uint32_t index = 0; index < views.size(); ++index)
            {
                images[index].imageView = views[index];
                images[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                writes[index] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, rFrame.frameDescriptorSet,
                    14 + index, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &images[index], nullptr, nullptr };
            }
            vkUpdateDescriptorSets(m_pDevice->GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::TransitionAmbientOcclusionImage(cVulkanImage& _rImage, bool _renderTarget)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = _renderTarget ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout           = _renderTarget ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask       = _renderTarget ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask       = _renderTarget ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = _rImage.GetImage();
        barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(m_frames[m_currentFrame].pCommandBuffer,
            _renderTarget ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            _renderTarget ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginAmbientOcclusionDraw()
    {
        if (!m_hasFrameStarted || m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("Ambient occlusion geometry must begin outside other render passes!");
        }

        const sVulkanFrame& rFrame  = m_frames[m_currentFrame];
        VkCommandBuffer commandBuffer = rFrame.pCommandBuffer;
        TransitionAmbientOcclusionImage(m_occlusionGeometry, true);

        VkMemoryBarrier depthBarrier{};
        depthBarrier.sType                      = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        depthBarrier.srcAccessMask              = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.dstAccessMask              = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        const VkPipelineStageFlags depthStages  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        vkCmdPipelineBarrier(commandBuffer, depthStages, depthStages, 0, 1, &depthBarrier, 0, nullptr, 0, nullptr);

        VkRenderingAttachmentInfo color{};
        color.sType         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color.imageView     = m_occlusionGeometry.GetImageView();
        color.imageLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depth{};
        depth.sType                     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth.imageView                 = m_occlusionDepth.GetImageView();
        depth.imageLayout               = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp                   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.clearValue.depthStencil   = { 1.0f, 0 };

        VkRenderingInfo rendering{};
        rendering.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering.renderArea.extent     = { m_occlusionGeometry.GetWidth(), m_occlusionGeometry.GetHeight() };
        rendering.layerCount            = 1;
        rendering.colorAttachmentCount  = 1;
        rendering.pColorAttachments     = &color;
        rendering.pDepthAttachment      = &depth;
        vkCmdBeginRendering(commandBuffer, &rendering);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(rendering.renderArea.extent.width),
            static_cast<float>(rendering.renderArea.extent.height), 0.0f, 1.0f };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &rendering.renderArea);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetNormalDepthPipeline());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPipelineLayout(),
            0, 1, &rFrame.frameDescriptorSet, 0, nullptr);
        m_renderPassType = sRenderPassType::AmbientOcclusion;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawAmbientOcclusionFilter(cVulkanImage& _rTarget, VkPipeline _pipeline)
    {
        const sVulkanFrame& rFrame = m_frames[m_currentFrame];
        VkCommandBuffer commandBuffer = rFrame.pCommandBuffer;
        TransitionAmbientOcclusionImage(_rTarget, true);

        VkRenderingAttachmentInfo color{};
        color.sType         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color.imageView     = _rTarget.GetImageView();
        color.imageLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo rendering{};
        rendering.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering.renderArea.extent     = { _rTarget.GetWidth(), _rTarget.GetHeight() };
        rendering.layerCount            = 1;
        rendering.colorAttachmentCount  = 1;
        rendering.pColorAttachments     = &color;

        vkCmdBeginRendering(commandBuffer, &rendering);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(_rTarget.GetWidth()), static_cast<float>(_rTarget.GetHeight()), 0.0f, 1.0f };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &rendering.renderArea);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPipelineLayout(),
            0, 1, &rFrame.frameDescriptorSet, 0, nullptr);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRendering(commandBuffer);
        TransitionAmbientOcclusionImage(_rTarget, false);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndAmbientOcclusionDraw()
    {
        if (m_renderPassType != sRenderPassType::AmbientOcclusion)
        {
            throw std::runtime_error("No ambient occlusion geometry pass is active!");
        }

        vkCmdEndRendering(m_frames[m_currentFrame].pCommandBuffer);
        m_renderPassType = sRenderPassType::None;
        TransitionAmbientOcclusionImage(m_occlusionGeometry, false);
        DrawAmbientOcclusionFilter(m_occlusionRaw, m_pPipeline->GetOcclusionPipeline());
        DrawAmbientOcclusionFilter(m_occlusionFiltered, m_pPipeline->GetOcclusionBlurPipeline());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateBloomBuffer()
    {
        const VkExtent2D extent = m_pSwapchain->GetExtent();
        uint32_t width = std::max(1u, (extent.width + 1) / 2);
        uint32_t height = std::max(1u, (extent.height + 1) / 2);

        VkCommandBuffer commandBuffer = m_pCommands->BeginSingleTimeCommands(*m_pDevice);

        const auto createImage = [&](cVulkanImage& _rImage)
        {
            _rImage.Create(
                *m_pDevice, width, height, VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);
            _rImage.TransitionLayout(
                *m_pDevice, commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        };

        for (uint32_t level = 0; level < c_bloomLevelCount; ++level)
        {
            createImage(m_bloomDownsampleImages[level]);

            if (level + 1 < c_bloomLevelCount)
            {
                createImage(m_bloomUpsampleImages[level]);
            }

            width = std::max(1u, (width + 1) / 2);
            height = std::max(1u, (height + 1) / 2);
        }

        m_pCommands->EndSingleTimeCommands(*m_pDevice, commandBuffer);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DestroyBloomBuffer()
    {
        for (cVulkanImage& rImage : m_bloomDownsampleImages)
        {
            rImage.Destroy(*m_pDevice);
        }

        for (cVulkanImage& rImage : m_bloomUpsampleImages)
        {
            rImage.Destroy(*m_pDevice);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreatePostProcessSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = VK_FILTER_LINEAR;
        samplerInfo.minFilter               = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.anisotropyEnable        = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.minLod                  = 0.0f;
        samplerInfo.maxLod                  = 0.0f;
        samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(m_pDevice->GetDevice(), &samplerInfo, nullptr, &m_postProcessSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create post-process sampler!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreatePostProcessDescriptorSet()
    {
        const VkDescriptorSetLayout layout = m_pPipeline->GetPostProcessDescriptorSetLayout();

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_pDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &layout;

        if (vkAllocateDescriptorSets(m_pDevice->GetDevice(), &allocInfo, &m_postProcessDescriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate post-process descriptor set!");
        }

        std::array<VkDescriptorSetLayout, c_bloomPassCount> bloomLayouts{};
        bloomLayouts.fill(layout);
        allocInfo.descriptorSetCount = c_bloomPassCount;
        allocInfo.pSetLayouts = bloomLayouts.data();

        if (vkAllocateDescriptorSets(m_pDevice->GetDevice(), &allocInfo, m_bloomDescriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate bloom descriptor sets!");
        }

        UpdatePostProcessDescriptorSet();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdatePostProcessDescriptorSet()
    {
        if (m_postProcessDescriptorSet == VK_NULL_HANDLE)
        {
            return;
        }

        UpdateAmbientOcclusionDescriptors();

        const auto updateSet = [&](VkDescriptorSet _set, VkImageView _source, VkImageView _bloom)
        {
            VkDescriptorImageInfo sceneImageInfo{};
            sceneImageInfo.imageView = _source;
            sceneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo samplerInfo{};
            samplerInfo.sampler = m_postProcessSampler;

            VkDescriptorImageInfo bloomImageInfo{};
            bloomImageInfo.imageView = _bloom;
            bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            std::array<VkWriteDescriptorSet, 4> writes{};
            writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _set, 0, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &sceneImageInfo, nullptr, nullptr };
            writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _set, 1, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerInfo, nullptr, nullptr };
            writes[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _set, 2, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &bloomImageInfo, nullptr, nullptr };
            writes[3] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _set, 3, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerInfo, nullptr, nullptr };
            vkUpdateDescriptorSets(m_pDevice->GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        };

        updateSet(m_postProcessDescriptorSet, m_colorBuffer.GetResolveImageView(), m_bloomUpsampleImages[0].GetImageView());

        for (uint32_t level = 0; level < c_bloomLevelCount; ++level)
        {
            const VkImageView source = level == 0
                ? m_colorBuffer.GetResolveImageView() : m_bloomDownsampleImages[level - 1].GetImageView();
            updateSet(m_bloomDescriptorSets[level], source, source);

            if (level + 1 < c_bloomLevelCount)
            {
                const VkImageView lowResolution = level + 2 == c_bloomLevelCount
                    ? m_bloomDownsampleImages[level + 1].GetImageView() : m_bloomUpsampleImages[level + 1].GetImageView();
                updateSet(m_bloomDescriptorSets[c_bloomLevelCount + level], m_bloomDownsampleImages[level].GetImageView(), lowResolution);
            }
        }
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

        if (frame.timestampQueryPool != VK_NULL_HANDLE && frame.timestampsSubmitted)
        {
            std::array<uint64_t, 6> timestamps{};
            const VkResult result = vkGetQueryPoolResults(
                device,
                frame.timestampQueryPool,
                0,
                static_cast<uint32_t>(timestamps.size()),
                sizeof(timestamps),
                timestamps.data(),
                sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT);

            if (result == VK_SUCCESS)
            {
                m_particleGpuMilliseconds = static_cast<double>((timestamps[5] - timestamps[4]) & m_timestampMask)
                    * m_timestampPeriod / 1000000.0;
                for (size_t pass = 0; pass < m_gpuPassMilliseconds.size(); ++pass)
                {
                    const uint64_t ticks = (timestamps[pass * 2 + 1] - timestamps[pass * 2]) & m_timestampMask;
                    m_gpuPassMilliseconds[pass] = static_cast<double>(ticks) * m_timestampPeriod / 1000000.0;
                }
            }
            else
            {
                m_gpuPassMilliseconds = { -1.0, -1.0 };
                m_particleGpuMilliseconds = -1.0;
            }
        }
        
         
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

        frame.healthBarCount = 0;
        frame.particleCount = 0;

        EnsureReflectionProbeResources();

        SelectActiveLights(_rCamera);
        UpdateFrameUniformBuffer(frame, _rCamera);

        VkCommandBuffer commandBuffer = frame.pCommandBuffer; 

        vkResetCommandBuffer(commandBuffer, 0); 

        VkCommandBufferBeginInfo beginInfo{}; 
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo)!= VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        if (frame.timestampQueryPool != VK_NULL_HANDLE)
        {
            vkCmdResetQueryPool(commandBuffer, frame.timestampQueryPool, 0, 6);
            frame.timestampsSubmitted = false;
        }

        Engine::GFX::ImGuiManager::BeginFrame();
        
        UpdateShadowBuffer(_rCamera);
        UpdateLightBuffer();
        UpdateActiveLightIndexBuffer();
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

        VkDevice        device          = m_pDevice->GetDevice();
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        EndDraw(pCommandBuffer, m_imageIndex);

        GFX::ImGuiManager::EndFrame(pCommandBuffer);

        EndUIDraw(pCommandBuffer, m_imageIndex);

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

        rFrame.timestampsSubmitted = rFrame.timestampQueryPool != VK_NULL_HANDLE;

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
        if (_rInstances.size() > c_maxNumberOfInstances)
        {
            throw std::length_error("Instance count exceeds the configured GPU instance buffer capacity!");
        }

        if (_rInstances.empty())
        {
            return;
        }

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

        const std::vector<sLight>& rLights  = LightManager::GetLights();
        const size_t lightCount             = std::min(rLights.size(), static_cast<size_t>(c_maxNumberOfLights));

        if (lightCount == 0)
        {
            return;
        }

        std::vector<sLightGPU> gpuLights(lightCount);

        for (size_t index = 0; index < lightCount; ++index)
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

    void cVulkanRenderer::SelectActiveLights(const cCamera& _rCamera)
    {
        m_previousActiveLightIndices = m_activeLightIndices;
        m_activeLightIndices.clear();
        m_activeLightCandidates.clear();

        const std::vector<sLight>& rLights = LightManager::GetLights();
        const uint32_t lightCount          = static_cast<uint32_t>(std::min(rLights.size(), static_cast<size_t>(c_maxNumberOfLights)));

        for (uint32_t lightIndex = 0; lightIndex < lightCount; ++lightIndex)
        {
            if (rLights[lightIndex].type != sLightType::Directional)
            {
                continue;
            }

            m_activeLightIndices.push_back(lightIndex);

            if (m_activeLightIndices.size() == c_maxNumberOfActiveLights)
            {
                return;
            }
        }

        const size_t directionalLightCount = m_activeLightIndices.size();

        float cameraPositionData[4];
        float cameraDirectionData[4];
        float projectionData[16];

        _rCamera.GetPosition(cameraPositionData);
        _rCamera.GetDirection(cameraDirectionData);

        const float width       = static_cast<float>(m_pSwapchain->GetExtent().width);
        const float height      = static_cast<float>(m_pSwapchain->GetExtent().height);
        const float aspectRatio = height > 0.0f ? width / height : 1.0f;

        _rCamera.GetProjectionMatrix(aspectRatio, projectionData);

        const Math::cVec3f cameraPosition =
        {
            cameraPositionData[0],
            cameraPositionData[1],
            cameraPositionData[2]
        };

        const Math::cVec3f cameraForward = Math::cVec3f(
            cameraDirectionData[0],
            cameraDirectionData[1],
            cameraDirectionData[2]
        ).normalized();

        const Math::cVec3f worldUp = { 0.0f, 1.0f, 0.0f };
        Math::cVec3f cameraRight = cameraForward.cross(worldUp).normalized();

        if (cameraRight.isZero())
        {
            cameraRight = { 1.0f, 0.0f, 0.0f };
        }

        const Math::cVec3f cameraUp = cameraRight.cross(cameraForward).normalized();

        const float xScale                  = std::abs(projectionData[0]);
        const float yScale                  = std::abs(projectionData[5]);
        const float tanHalfHorizontalFov     = xScale > 0.000001f ? 1.0f / xScale : 1.0f;
        const float tanHalfVerticalFov       = yScale > 0.000001f ? 1.0f / yScale : 1.0f;
        const float horizontalRadiusScale = std::sqrt(1.0f + tanHalfHorizontalFov * tanHalfHorizontalFov);
        const float verticalRadiusScale   = std::sqrt(1.0f + tanHalfVerticalFov * tanHalfVerticalFov);

        for (uint32_t lightIndex = 0; lightIndex < lightCount; ++lightIndex)
        {
            const sLight& rLight = rLights[lightIndex];

            if (rLight.type == sLightType::Directional)
            {
                continue;
            }

            const float radius = std::max(rLight.radius, 0.0f);

            if (radius <= 0.0f || rLight.intensity <= 0.0f)
            {
                continue;
            }

            const Math::cVec3f cameraToLight = rLight.position - cameraPosition;
            const float viewDepth = cameraToLight.dot(cameraForward);

            if (viewDepth + radius < _rCamera.GetNearPlane() || viewDepth - radius > _rCamera.GetFarPlane())
            {
                continue;
            }

            const float horizontalDistance = std::abs(cameraToLight.dot(cameraRight));
            const float verticalDistance   = std::abs(cameraToLight.dot(cameraUp));

            if (horizontalDistance > viewDepth * tanHalfHorizontalFov + radius * horizontalRadiusScale
                || verticalDistance > viewDepth * tanHalfVerticalFov + radius * verticalRadiusScale)
            {
                continue;
            }

            const float distanceToLight     = std::sqrt(cameraToLight.lengthSquared());
            const float distanceToInfluence = std::max(distanceToLight - radius, 1.0f);
            const float projectedRadius     = radius / distanceToInfluence;
            const float brightness          = std::max({ rLight.color.x(), rLight.color.y(), rLight.color.z() }) * rLight.intensity;
            const float heightDifference    = std::abs(rLight.position.y() - cameraPosition.y());
            const float heightReference     = std::max(radius * 0.5f, 1.0f);
            const float normalizedHeight    = heightDifference / heightReference;
            const float heightPreference    = 1.0f / (1.0f + normalizedHeight * normalizedHeight);

            const float priority = brightness * projectedRadius * projectedRadius * heightPreference;

            m_activeLightCandidates.emplace_back(priority, lightIndex);
        }

        std::sort(m_activeLightCandidates.begin(), m_activeLightCandidates.end(), [](const auto& _rLeft, const auto& _rRight)
        {
            if (_rLeft.first != _rRight.first)
            {
                return _rLeft.first > _rRight.first;
            }

            return _rLeft.second < _rRight.second;
        });

        const size_t remainingLightCount = c_maxNumberOfActiveLights - m_activeLightIndices.size();

        for (uint32_t previousLightIndex : m_previousActiveLightIndices)
        {
            if (m_activeLightIndices.size() - directionalLightCount == remainingLightCount)
            {
                break;
            }

            const auto candidate = std::find_if(m_activeLightCandidates.begin(), m_activeLightCandidates.end(), [previousLightIndex](const auto& _rCandidate)
            {
                return _rCandidate.second == previousLightIndex;
            });

            if (candidate != m_activeLightCandidates.end())
            {
                m_activeLightIndices.push_back(previousLightIndex);
            }
        }

        for (const auto& rCandidate : m_activeLightCandidates)
        {
            if (m_activeLightIndices.size() == c_maxNumberOfActiveLights)
            {
                break;
            }

            if (std::find(m_activeLightIndices.begin(), m_activeLightIndices.end(), rCandidate.second) == m_activeLightIndices.end())
            {
                m_activeLightIndices.push_back(rCandidate.second);
            }
        }

        constexpr uint32_t c_maxReplacementsPerFrame = 2;
        constexpr float c_replacementPriorityFactor  = 1.5f;

        for (uint32_t replacementIndex = 0; replacementIndex < c_maxReplacementsPerFrame; ++replacementIndex)
        {
            const auto challenger = std::find_if(m_activeLightCandidates.begin(), m_activeLightCandidates.end(), [this](const auto& _rCandidate)
            {
                return std::find(m_activeLightIndices.begin(), m_activeLightIndices.end(), _rCandidate.second) == m_activeLightIndices.end();
            });

            if (challenger == m_activeLightCandidates.end())
            {
                break;
            }

            size_t lowestPriorityPosition = directionalLightCount;
            float lowestPriority = std::numeric_limits<float>::max();

            for (size_t activePosition = directionalLightCount; activePosition < m_activeLightIndices.size(); ++activePosition)
            {
                const uint32_t activeLightIndex = m_activeLightIndices[activePosition];
                const auto activeCandidate = std::find_if(m_activeLightCandidates.begin(), m_activeLightCandidates.end(), [activeLightIndex](const auto& _rCandidate)
                {
                    return _rCandidate.second == activeLightIndex;
                });

                if (activeCandidate != m_activeLightCandidates.end() && activeCandidate->first < lowestPriority)
                {
                    lowestPriorityPosition = activePosition;
                    lowestPriority = activeCandidate->first;
                }
            }

            if (lowestPriorityPosition >= m_activeLightIndices.size()
                || challenger->first <= lowestPriority * c_replacementPriorityFactor)
            {
                break;
            }

            m_activeLightIndices[lowestPriorityPosition] = challenger->second;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateActiveLightIndexBuffer()
    {
        if (m_activeLightIndices.empty())
        {
            return;
        }

        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;
        const VkDeviceSize indexDataSize = sizeof(uint32_t) * m_activeLightIndices.size();

        rFrame.activeLightIndexStagingBuffer.Write(m_activeLightIndices.data(), indexDataSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size      = indexDataSize;

        vkCmdCopyBuffer(
            pCommandBuffer,
            rFrame.activeLightIndexStagingBuffer.GetBuffer(),
            rFrame.activeLightIndexBuffer.GetBuffer(),
            1,
            &copyRegion
        );

        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = rFrame.activeLightIndexBuffer.GetBuffer();
        barrier.offset              = 0;
        barrier.size                = indexDataSize;

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

        if (rMaterials.size() > c_maxNumberOfMaterials)
        {
            throw std::length_error("Material count exceeds the GPU buffer capacity!");
        }

        // BeginFrame waited for this frame's fence before the CPU reuses its staging memory.
        cVulkanBuffer& rStagingBuffer = m_materialStagingBuffers[m_currentFrame];
        rStagingBuffer.Write(rMaterials.data(), materialSize);

        VkBufferMemoryBarrier beforeUpload{};
        beforeUpload.sType                  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        beforeUpload.srcAccessMask          = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        beforeUpload.dstAccessMask          = VK_ACCESS_TRANSFER_WRITE_BIT;
        beforeUpload.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
        beforeUpload.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
        beforeUpload.buffer                 = m_materialBuffer.GetBuffer();
        beforeUpload.size                   = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            pCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &beforeUpload, 0, nullptr);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset    = 0;
        copyRegion.dstOffset    = 0;
        copyRegion.size         = materialSize;

        vkCmdCopyBuffer(pCommandBuffer, rStagingBuffer.GetBuffer(), m_materialBuffer.GetBuffer(), 1, &copyRegion);
    
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

        const std::vector<sLight>& rLights = LightManager::GetLights();
        const size_t lightCount            = std::min(rLights.size(), static_cast<size_t>(c_maxNumberOfLights));

        m_shadowData.clear();
        m_shadowData.reserve(lightCount);

        Math::cVec3f shadowCenter =
        {
            cameraPosition[0],
            cameraPosition[1],
            cameraPosition[2]
        };

        m_lightShadowIndices.assign(lightCount, -1);


        uint32_t nextLayer = 0;

        for (uint32_t lightIndex = 0; lightIndex < static_cast<uint32_t>(lightCount); ++lightIndex)
        {
            const sLight& rLight        = rLights[lightIndex];
            uint32_t requiredLayers     = 0;

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

    void cVulkanRenderer::UpdateReflectionProbeDescriptors(sVulkanFrame& _rFrame, const std::vector<ReflectionProbeHandle>& _rActiveProbeHandles)
    {
        std::array<VkDescriptorImageInfo, c_maxNumberOfActiveReflectionProbes> reflectionProbeImageInfos{};

        for (uint32_t slotIndex = 0; slotIndex < c_maxNumberOfActiveReflectionProbes; ++slotIndex)
        {
            reflectionProbeImageInfos[slotIndex].sampler = VK_NULL_HANDLE;
            reflectionProbeImageInfos[slotIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (slotIndex < _rActiveProbeHandles.size())
            {
                const ReflectionProbeHandle probeHandle = _rActiveProbeHandles[slotIndex];

                reflectionProbeImageInfos[slotIndex].imageView = m_vulkanReflectionProbes[probeHandle]->GetPrefilteredImageView();
            }
            else
            {
                reflectionProbeImageInfos[slotIndex].imageView = m_environment.GetImageView();
            }
        }

        VkWriteDescriptorSet descriptorWrite{};

        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = _rFrame.frameDescriptorSet;
        descriptorWrite.dstBinding      = 12;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = c_maxNumberOfActiveReflectionProbes;
        descriptorWrite.pImageInfo      = reflectionProbeImageInfos.data();

        vkUpdateDescriptorSets(m_pDevice->GetDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginShadowRendering()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        if (rFrame.timestampQueryPool != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(pCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, rFrame.timestampQueryPool, 0);
        }

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

        if (rFrame.timestampQueryPool != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(pCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, rFrame.timestampQueryPool, 1);
        }
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

    uint32_t cVulkanRenderer::GetReflectionProbeCount() const
    {
        return ReflectionProbeManager::GetProbeCount();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateReflectionProbePrefilterDescriptorSets(uint32_t _firstProbeIndex)
    {
        const uint32_t reflectionProbeCount = ReflectionProbeManager::GetProbeCount();

        if (_firstProbeIndex >= reflectionProbeCount)
        {
            return;
        }

        const uint32_t descriptorSetCount = reflectionProbeCount - _firstProbeIndex;

        m_reflectionProbePrefilterDescriptorSets.resize(reflectionProbeCount, VK_NULL_HANDLE);

        std::vector<VkDescriptorSetLayout> layouts(descriptorSetCount, m_pPipeline->GetReflectionProbePrefilterDescriptorSetLayout());
        std::vector<VkDescriptorSet> descriptorSets(descriptorSetCount, VK_NULL_HANDLE);

        VkDescriptorSetAllocateInfo allocInfo{};

        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_pDescriptorPool;
        allocInfo.descriptorSetCount = descriptorSetCount;
        allocInfo.pSetLayouts        = layouts.data();

        if (vkAllocateDescriptorSets(m_pDevice->GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate reflection probe prefilter descriptor sets!");
        }

        for (ReflectionProbeHandle probeHandle = _firstProbeIndex; probeHandle < reflectionProbeCount; ++probeHandle)
        {
            m_reflectionProbePrefilterDescriptorSets[probeHandle] = descriptorSets[probeHandle - _firstProbeIndex];
            UpdateReflectionProbePrefilterDescriptorSet(probeHandle);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateReflectionProbePrefilterDescriptorSet(uint32_t _probeIndex)
    {
        VkDescriptorImageInfo captureImageInfo{};

        captureImageInfo.sampler        = VK_NULL_HANDLE;
        captureImageInfo.imageView      = m_vulkanReflectionProbes[_probeIndex]->GetCaptureImageView();
        captureImageInfo.imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo captureSamplerInfo{};

        captureSamplerInfo.sampler      = m_vulkanReflectionProbes[_probeIndex]->GetSampler();
        captureSamplerInfo.imageView    = VK_NULL_HANDLE;
        captureSamplerInfo.imageLayout  = VK_IMAGE_LAYOUT_UNDEFINED;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = m_reflectionProbePrefilterDescriptorSets[_probeIndex];
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo      = &captureImageInfo;

        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = m_reflectionProbePrefilterDescriptorSets[_probeIndex];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo      = &captureSamplerInfo;

        vkUpdateDescriptorSets(m_pDevice->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EnsureReflectionProbeResources()
    {
        const uint32_t requestedProbeCount = ReflectionProbeManager::GetProbeCount();
        const uint32_t existingProbeCount = static_cast<uint32_t>(m_vulkanReflectionProbes.size());

        if (requestedProbeCount > c_maxNumberOfReflectionProbes)
        {
            throw std::runtime_error("Reflection probe count exceeds the streaming probe capacity!");
        }

        m_reflectionProbeCount = requestedProbeCount;
        const uint32_t resourceCount = std::max(requestedProbeCount, existingProbeCount);
        m_reflectionProbeCaptureLayouts.resize(resourceCount, VK_IMAGE_LAYOUT_UNDEFINED);
        m_reflectionProbePrefilteredLayouts.resize(resourceCount, VK_IMAGE_LAYOUT_UNDEFINED);
        m_vulkanReflectionProbes.reserve(requestedProbeCount);

        uint32_t maximumResolution = m_reflectionProbeDepthResolution;
        bool waitedForResources = false;

        for (ReflectionProbeHandle probeHandle = 0; probeHandle < std::min(requestedProbeCount, existingProbeCount); ++probeHandle)
        {
            const sReflectionProbe& rProbe = ReflectionProbeManager::GetProbe(probeHandle);
            cVulkanReflectionProbe& rVulkanProbe = *m_vulkanReflectionProbes[probeHandle];

            if (!rProbe.active || rProbe.resolution == rVulkanProbe.GetResolution())
            {
                continue;
            }

            // A streamed handle can be reused with a different capture resolution.
            if (!waitedForResources)
            {
                m_pDevice->WaitIdle();
                waitedForResources = true;
            }

            rVulkanProbe.Destroy(*m_pDevice);
            rVulkanProbe.Create(*m_pDevice, rProbe.resolution);
            m_reflectionProbeCaptureLayouts[probeHandle] = VK_IMAGE_LAYOUT_UNDEFINED;
            m_reflectionProbePrefilteredLayouts[probeHandle] = VK_IMAGE_LAYOUT_UNDEFINED;
            ReflectionProbeManager::SetProbeDirty(probeHandle, true);
            UpdateReflectionProbePrefilterDescriptorSet(probeHandle);
            maximumResolution = std::max(maximumResolution, rProbe.resolution);
        }

        for (ReflectionProbeHandle probeHandle = existingProbeCount; probeHandle < requestedProbeCount; ++probeHandle)
        {
            const sReflectionProbe& rProbe = ReflectionProbeManager::GetProbe(probeHandle);
            std::unique_ptr<cVulkanReflectionProbe> pProbe = std::make_unique<cVulkanReflectionProbe>();

            pProbe->Create(*m_pDevice, rProbe.resolution);

            maximumResolution = std::max(maximumResolution, rProbe.resolution);
            m_vulkanReflectionProbes.push_back(std::move(pProbe));
        }

        if (maximumResolution > m_reflectionProbeDepthResolution)
        {
            if (!waitedForResources)
            {
                m_pDevice->WaitIdle();
            }
            m_reflectionProbeDepthImage.Destroy(*m_pDevice);
            m_reflectionProbeDepthImage.Create(
                *m_pDevice,
                maximumResolution,
                maximumResolution,
                VK_FORMAT_D32_SFLOAT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                VK_SAMPLE_COUNT_1_BIT
            );

            m_reflectionProbeDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            m_reflectionProbeDepthResolution = maximumResolution;
        }

        CreateReflectionProbePrefilterDescriptorSets(existingProbeCount);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::GenerateReflectionProbeCaptureMipmaps()
    {
        const uint32_t probeIndex = m_activeReflectionProbeIndex;

        if (probeIndex == UINT32_MAX)
        {
            throw std::runtime_error("No active reflection probe!");
        }

        cVulkanReflectionProbe& rVulkanProbe = *m_vulkanReflectionProbes[probeIndex];

        if (!m_hasFrameStarted)
        {
            throw std::runtime_error("GenerateReflectionProbeCaptureMipmaps() called outside of a frame!");
        }

        if (m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("GenerateReflectionProbeCaptureMipmaps() called while another render pass is active!");
        }

        sVulkanFrame& rFrame = m_frames[m_currentFrame];

        VkCommandBuffer pCommandBuffer = rFrame.pCommandBuffer;
        VkImage pImage = rVulkanProbe.GetCaptureImage().GetImage();

        const uint32_t mipLevels = rVulkanProbe.GetMipLevels();

        if (mipLevels <= 1)
        {
            return;
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Mip 0: COLOR_ATTACHMENT -> TRANSFER_SRC
        // -------------------------------------------------------------------------------------------------------------------------

        VkImageMemoryBarrier mip0Barrier{};

        mip0Barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        mip0Barrier.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        mip0Barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mip0Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mip0Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mip0Barrier.image               = pImage;

        mip0Barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        mip0Barrier.subresourceRange.baseMipLevel   = 0;
        mip0Barrier.subresourceRange.levelCount     = 1;
        mip0Barrier.subresourceRange.baseArrayLayer = 0;
        mip0Barrier.subresourceRange.layerCount     = 6;

        mip0Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        mip0Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &mip0Barrier
        );

        // -------------------------------------------------------------------------------------------------------------------------
        // Remaining mips: COLOR_ATTACHMENT -> TRANSFER_DST
        //
        // They were not rendered into, but BeginReflectionProbeRendering() put the complete image into COLOR_ATTACHMENT_OPTIMAL.
        // -------------------------------------------------------------------------------------------------------------------------

        VkImageMemoryBarrier destinationBarrier{};

        destinationBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        destinationBarrier.oldLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        destinationBarrier.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        destinationBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        destinationBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        destinationBarrier.image                = pImage;

        destinationBarrier.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        destinationBarrier.subresourceRange.baseMipLevel    = 1;
        destinationBarrier.subresourceRange.levelCount      = mipLevels - 1;
        destinationBarrier.subresourceRange.baseArrayLayer  = 0;
        destinationBarrier.subresourceRange.layerCount      = 6;

        destinationBarrier.srcAccessMask = 0;
        destinationBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &destinationBarrier
        );

        // -------------------------------------------------------------------------------------------------------------------------
        // Generate mip chain
        // -------------------------------------------------------------------------------------------------------------------------

        int32_t sourceWidth  = static_cast<int32_t>(rVulkanProbe.GetResolution());
        int32_t sourceHeight = static_cast<int32_t>(rVulkanProbe.GetResolution());

        for (uint32_t mipLevel = 1; mipLevel < mipLevels; ++mipLevel)
        {
            const int32_t destinationWidth  = sourceWidth  > 1 ? sourceWidth  / 2 : 1;
            const int32_t destinationHeight = sourceHeight > 1 ? sourceHeight / 2 : 1;

            VkImageBlit blit{};

            blit.srcSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel        = mipLevel - 1;
            blit.srcSubresource.baseArrayLayer  = 0;
            blit.srcSubresource.layerCount      = 6;

            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { sourceWidth, sourceHeight, 1 };

            blit.dstSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel        = mipLevel;
            blit.dstSubresource.baseArrayLayer  = 0;
            blit.dstSubresource.layerCount      = 6;

            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { destinationWidth, destinationHeight, 1 };

            vkCmdBlitImage(
                pCommandBuffer,
                pImage,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                pImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &blit,
                VK_FILTER_LINEAR
            );

            // ---------------------------------------------------------------------------------------------------------------------
            // Newly generated mip becomes source for the next level
            // ---------------------------------------------------------------------------------------------------------------------

            VkImageMemoryBarrier mipBarrier{};

            mipBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            mipBarrier.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            mipBarrier.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mipBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            mipBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            mipBarrier.image                = pImage;

            mipBarrier.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            mipBarrier.subresourceRange.baseMipLevel    = mipLevel;
            mipBarrier.subresourceRange.levelCount      = 1;
            mipBarrier.subresourceRange.baseArrayLayer  = 0;
            mipBarrier.subresourceRange.layerCount      = 6;

            mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            mipBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                pCommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &mipBarrier
            );

            sourceWidth  = destinationWidth;
            sourceHeight = destinationHeight;
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Complete mip chain -> SHADER_READ_ONLY
        //
        // At this point every mip is TRANSFER_SRC_OPTIMAL.
        // -------------------------------------------------------------------------------------------------------------------------

        VkImageMemoryBarrier shaderReadBarrier{};

        shaderReadBarrier.sType                 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        shaderReadBarrier.oldLayout             = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        shaderReadBarrier.newLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shaderReadBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
        shaderReadBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
        shaderReadBarrier.image                 = pImage;

        shaderReadBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        shaderReadBarrier.subresourceRange.baseMipLevel   = 0;
        shaderReadBarrier.subresourceRange.levelCount     = mipLevels;
        shaderReadBarrier.subresourceRange.baseArrayLayer = 0;
        shaderReadBarrier.subresourceRange.layerCount     = 6;

        shaderReadBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        shaderReadBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &shaderReadBarrier
        );

        m_reflectionProbeCaptureLayouts[probeIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::PrefilterReflectionProbe(uint32_t _probeIndex)
    {
        if (_probeIndex >= ReflectionProbeManager::GetProbeCount())
        {
            throw std::runtime_error("Invalid reflection probe index!");
        }

        cVulkanReflectionProbe& rVulkanProbe = *m_vulkanReflectionProbes[_probeIndex];

        if (!m_hasFrameStarted)
        {
            throw std::runtime_error("PrefilterReflectionProbe() called outside of a frame!");
        }

        if (m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("PrefilterReflectionProbe() called while another render pass is active!");
        }

        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        const uint32_t mipLevels = rVulkanProbe.GetMipLevels();

        // -------------------------------------------------------------------------------------------------------------------------
        // Transition complete prefiltered cubemap -> COLOR_ATTACHMENT_OPTIMAL
        // -------------------------------------------------------------------------------------------------------------------------

        VkImageMemoryBarrier beginBarrier{};

        beginBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        beginBarrier.oldLayout           = m_reflectionProbePrefilteredLayouts[_probeIndex];
        beginBarrier.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        beginBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        beginBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        beginBarrier.image               = rVulkanProbe.GetPrefilteredImage().GetImage();

        beginBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        beginBarrier.subresourceRange.baseMipLevel   = 0;
        beginBarrier.subresourceRange.levelCount     = mipLevels;
        beginBarrier.subresourceRange.baseArrayLayer = 0;
        beginBarrier.subresourceRange.layerCount     = 6;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        if (m_reflectionProbePrefilteredLayouts[_probeIndex] == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            beginBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            beginBarrier.srcAccessMask = 0;
        }

        beginBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            sourceStage,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &beginBarrier
        );

        m_reflectionProbePrefilteredLayouts[_probeIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // -------------------------------------------------------------------------------------------------------------------------
        // Pipeline + descriptor
        // -------------------------------------------------------------------------------------------------------------------------

        vkCmdBindPipeline(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetReflectionProbePrefilterPipeline());

        vkCmdBindDescriptorSets(
            pCommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pPipeline->GetReflectionProbePrefilterPipelineLayout(),
            0,
            1,
            &m_reflectionProbePrefilterDescriptorSets[_probeIndex],
            0,
            nullptr
        );

        // -------------------------------------------------------------------------------------------------------------------------
        // Render every mip and every cubemap face
        // -------------------------------------------------------------------------------------------------------------------------

        for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
        {
            const uint32_t mipResolution = rVulkanProbe.GetMipResolution(mipLevel);

            const float roughness = mipLevels > 1 ? static_cast<float>(mipLevel) / static_cast<float>(mipLevels - 1) : 0.0f;

            for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                VkRenderingAttachmentInfo colorAttachment{};

                colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                colorAttachment.imageView   = rVulkanProbe.GetPrefilteredFaceMipImageView(faceIndex, mipLevel);
                colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

                VkRenderingInfo renderingInfo{};

                renderingInfo.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
                renderingInfo.renderArea.offset     = { 0, 0 };
                renderingInfo.renderArea.extent     = { mipResolution, mipResolution };
                renderingInfo.layerCount            = 1;
                renderingInfo.colorAttachmentCount  = 1;
                renderingInfo.pColorAttachments     = &colorAttachment;
                renderingInfo.pDepthAttachment      = nullptr;
                renderingInfo.pStencilAttachment    = nullptr;

                vkCmdBeginRendering(pCommandBuffer, &renderingInfo);

                // -----------------------------------------------------------------------------------------------------------------
                // Viewport
                // -----------------------------------------------------------------------------------------------------------------

                VkViewport viewport{};

                viewport.x        = 0.0f;
                viewport.y        = 0.0f;
                viewport.width    = static_cast<float>(mipResolution);
                viewport.height   = static_cast<float>(mipResolution);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                vkCmdSetViewport(pCommandBuffer, 0, 1, &viewport);

                // -----------------------------------------------------------------------------------------------------------------
                // Scissor
                // -----------------------------------------------------------------------------------------------------------------

                VkRect2D scissor{};

                scissor.offset = { 0, 0 };
                scissor.extent = { mipResolution, mipResolution };

                vkCmdSetScissor(pCommandBuffer, 0, 1, &scissor);

                // -----------------------------------------------------------------------------------------------------------------
                // Push constants
                // -----------------------------------------------------------------------------------------------------------------

                sReflectionProbePrefilterPushConstants pushConstants{};

                pushConstants.faceIndex         = faceIndex;
                pushConstants.roughness         = roughness;
                pushConstants.sampleCount       = mipLevel == 0 ? 1u : std::min(1024u, 64u << std::min(mipLevel, 4u));
                pushConstants.captureResolution = static_cast<float>(rVulkanProbe.GetResolution());

                vkCmdPushConstants(
                    pCommandBuffer,
                    m_pPipeline->GetReflectionProbePrefilterPipelineLayout(),
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(sReflectionProbePrefilterPushConstants),
                    &pushConstants
                );

                // -----------------------------------------------------------------------------------------------------------------
                // Fullscreen triangle
                // -----------------------------------------------------------------------------------------------------------------

                vkCmdDraw(pCommandBuffer, 3, 1, 0, 0);

                vkCmdEndRendering(pCommandBuffer);
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // COLOR_ATTACHMENT -> SHADER_READ_ONLY
        // -------------------------------------------------------------------------------------------------------------------------

        VkImageMemoryBarrier endBarrier{};

        endBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        endBarrier.oldLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        endBarrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        endBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        endBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        endBarrier.image                = rVulkanProbe.GetPrefilteredImage().GetImage();

        endBarrier.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        endBarrier.subresourceRange.baseMipLevel    = 0;
        endBarrier.subresourceRange.levelCount      = mipLevels;
        endBarrier.subresourceRange.baseArrayLayer  = 0;
        endBarrier.subresourceRange.layerCount      = 6;

        endBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        endBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            pCommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &endBarrier
        );

        m_reflectionProbePrefilteredLayouts[_probeIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        ReflectionProbeManager::SetProbeDirty(_probeIndex, false);
        m_activeReflectionProbeIndex = UINT32_MAX;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateHealthBars(std::span<const sHealthBarData> _healthBars)
    {
        if (!m_hasFrameStarted || m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("Health bars must be uploaded before rendering begins!");
        }

        if (_healthBars.size() > c_maxNumberOfHealthBars)
        {
            throw std::length_error("Health bar count exceeds the GPU buffer capacity!");
        }

        sVulkanFrame& rFrame = m_frames[m_currentFrame];
        rFrame.healthBarCount = static_cast<uint32_t>(_healthBars.size());

        if (!_healthBars.empty())
        {
            rFrame.healthBarBuffer.Write(_healthBars.data(), _healthBars.size_bytes());
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::UpdateParticles(std::span<const sParticleData> _particles)
    {
        if (!m_hasFrameStarted || m_renderPassType != sRenderPassType::None)
            throw std::runtime_error("Particles must be uploaded before rendering begins!");
        if (_particles.size() > c_maxParticleDraws)
            throw std::length_error("Particle count exceeds the GPU buffer capacity!");

        sVulkanFrame& frame = m_frames[m_currentFrame];
        frame.particleCount = static_cast<uint32_t>(_particles.size());
        if (!_particles.empty())
            frame.particleBuffer.Write(_particles.data(), _particles.size_bytes());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawParticles()
    {
        if (!m_hasFrameStarted || m_renderPassType != sRenderPassType::Main)
            throw std::runtime_error("Particles can only be drawn in the main pass!");

        const sVulkanFrame& frame = m_frames[m_currentFrame];

        if (frame.timestampQueryPool != VK_NULL_HANDLE)
            vkCmdWriteTimestamp(frame.pCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.timestampQueryPool, 4);

        if (frame.particleCount == 0)
        {
            if (frame.timestampQueryPool != VK_NULL_HANDLE)
                vkCmdWriteTimestamp(frame.pCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.timestampQueryPool, 5);
            return;
        }

        const VkBuffer     buffer = frame.particleBuffer.GetBuffer();
        const VkDeviceSize offset = 0;

        vkCmdBindPipeline(frame.pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetParticlePipeline());

        vkCmdBindDescriptorSets(frame.pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            m_pPipeline->GetPipelineLayout(), 0, 1, &frame.frameDescriptorSet, 0, nullptr);
        
        vkCmdBindVertexBuffers(frame.pCommandBuffer, 0, 1, &buffer, &offset);
        vkCmdDraw(frame.pCommandBuffer, 6, frame.particleCount, 0, 0);
        if (frame.timestampQueryPool != VK_NULL_HANDLE)
            vkCmdWriteTimestamp(frame.pCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.timestampQueryPool, 5);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawHealthBars()
    {
        if (!m_hasFrameStarted || m_renderPassType != sRenderPassType::Main)
        {
            throw std::runtime_error("Health bars can only be drawn in the main pass!");
        }

        const sVulkanFrame& rFrame = m_frames[m_currentFrame];

        if (rFrame.healthBarCount == 0)
        {
            return;
        }

        const VkCommandBuffer   commandBuffer = rFrame.pCommandBuffer;
        const VkBuffer          buffer        = rFrame.healthBarBuffer.GetBuffer();
        const VkDeviceSize      offset        = 0;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetHealthBarPipeline());
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPipelineLayout(),
            0, 1, &rFrame.frameDescriptorSet, 0, nullptr);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &offset);
        vkCmdDraw(commandBuffer, 6, rFrame.healthBarCount, 0, 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginDraw()
    {
        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        if (rFrame.timestampQueryPool != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(pCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, rFrame.timestampQueryPool, 2);
        }

        m_renderPassType = sRenderPassType::Main;

        VkExtent2D  extent             = m_pSwapchain->GetExtent();

        VkImageMemoryBarrier sceneBarrier{};

        sceneBarrier.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        sceneBarrier.oldLayout                          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        sceneBarrier.newLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        sceneBarrier.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        sceneBarrier.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        sceneBarrier.image                              = m_colorBuffer.GetResolveImage();
        sceneBarrier.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        sceneBarrier.subresourceRange.baseMipLevel      = 0;
        sceneBarrier.subresourceRange.levelCount        = 1;
        sceneBarrier.subresourceRange.baseArrayLayer    = 0;
        sceneBarrier.subresourceRange.layerCount        = 1;
        sceneBarrier.srcAccessMask                      = VK_ACCESS_SHADER_READ_BIT;
        sceneBarrier.dstAccessMask                      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(pCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &sceneBarrier);

        VkClearValue clearValue{};

        clearValue.color.float32[0] = m_backgroundColor[0];
        clearValue.color.float32[1] = m_backgroundColor[1];
        clearValue.color.float32[2] = m_backgroundColor[2];
        clearValue.color.float32[3] = m_backgroundColor[3];

        VkRenderingAttachmentInfo colorAttachment{};

        colorAttachment.sType               = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView           = m_colorBuffer.GetImageView();
        colorAttachment.imageLayout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp              = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp             = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue          = clearValue;
        colorAttachment.resolveMode         = VK_RESOLVE_MODE_AVERAGE_BIT;
        colorAttachment.resolveImageView    = m_colorBuffer.GetResolveImageView();
        colorAttachment.resolveImageLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (m_pDevice->GetMSAASamples() == VK_SAMPLE_COUNT_1_BIT)
        {
            colorAttachment.imageView = m_colorBuffer.GetResolveImageView();
            colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
            colorAttachment.resolveImageView = VK_NULL_HANDLE;
        }

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

    bool cVulkanRenderer::NeedsReflectionProbeUpdate(uint32_t _probeIndex) const
    {
        if (_probeIndex >= ReflectionProbeManager::GetProbeCount()
            || _probeIndex >= m_vulkanReflectionProbes.size())
        {
            return false;
        }

        const sReflectionProbe& rProbe = ReflectionProbeManager::GetProbe(_probeIndex);

        return rProbe.active
            && rProbe.dirty
            && std::find(m_activeReflectionProbeHandles.begin(), m_activeReflectionProbeHandles.end(), _probeIndex) != m_activeReflectionProbeHandles.end();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginReflectionProbeRendering(uint32_t _probeIndex)
    {
        if (_probeIndex >= ReflectionProbeManager::GetProbeCount())
        {
            throw std::runtime_error("Invalid reflection probe handle!");
        }

        if (!m_hasFrameStarted)
        {
            throw std::runtime_error("BeginReflectionProbeRendering() called outside of a frame!");
        }

        if (m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("BeginReflectionProbeRendering() called while another render pass is active!");
        }

        m_activeReflectionProbeIndex = _probeIndex;

        cVulkanReflectionProbe& rVulkanProbe = *m_vulkanReflectionProbes[_probeIndex];

        sVulkanFrame&   rFrame          = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer  = rFrame.pCommandBuffer;

        VkImageMemoryBarrier colorBarrier{};

        colorBarrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        colorBarrier.oldLayout              = m_reflectionProbeCaptureLayouts[_probeIndex];
        colorBarrier.newLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorBarrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
        colorBarrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
        colorBarrier.image                  = rVulkanProbe.GetCaptureImage().GetImage();

        colorBarrier.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        colorBarrier.subresourceRange.baseMipLevel      = 0;
        colorBarrier.subresourceRange.levelCount        = rVulkanProbe.GetMipLevels();
        colorBarrier.subresourceRange.baseArrayLayer    = 0;
        colorBarrier.subresourceRange.layerCount        = 6;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        if (m_reflectionProbeCaptureLayouts[_probeIndex] == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            colorBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            colorBarrier.srcAccessMask = 0;
        }

        colorBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(pCommandBuffer, sourceStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);

        m_reflectionProbeCaptureLayouts[_probeIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (m_reflectionProbeDepthLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            m_reflectionProbeDepthImage.TransitionLayout(*m_pDevice, pCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
            m_reflectionProbeDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginReflectionProbeDraw(uint32_t _faceIndex)
    {
        if (m_activeReflectionProbeIndex == UINT32_MAX)
        {
            throw std::runtime_error("BeginReflectionProbeDraw() called without active reflection probe!");
        }

        const uint32_t probeIndex = m_activeReflectionProbeIndex;

        const sReflectionProbe& rProbe = ReflectionProbeManager::GetProbe(probeIndex);
        cVulkanReflectionProbe& rVulkanProbe = *m_vulkanReflectionProbes[probeIndex];

        if (_faceIndex >= 6)
        {
            throw std::runtime_error("Invalid reflection probe face index!");
        }

        if (m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("BeginReflectionProbeDraw() called while another render pass is active!");
        }

        sVulkanFrame& rFrame = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer = rFrame.pCommandBuffer;

        m_renderPassType = sRenderPassType::ReflectionProbe;

        // -------------------------------------------------------------------------------------------------------------------------
        // Attachments
        // -------------------------------------------------------------------------------------------------------------------------

        VkClearValue clearColor{};

        clearColor.color.float32[0] = 0.0f;
        clearColor.color.float32[1] = 0.0f;
        clearColor.color.float32[2] = 0.0f;
        clearColor.color.float32[3] = 1.0f;

        VkRenderingAttachmentInfo colorAttachment{};

        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = rVulkanProbe.GetCaptureFaceImageView(_faceIndex);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue  = clearColor;

        VkRenderingAttachmentInfo depthAttachment{};

        depthAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView               = m_reflectionProbeDepthImage.GetImageView();
        depthAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkExtent2D extent =
        {
            rVulkanProbe.GetResolution(),
            rVulkanProbe.GetResolution()
        };

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

        // -------------------------------------------------------------------------------------------------------------------------
        // Viewport / Scissor
        // -------------------------------------------------------------------------------------------------------------------------

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

        // -------------------------------------------------------------------------------------------------------------------------
        // Pipeline / Descriptors
        // -------------------------------------------------------------------------------------------------------------------------

        vkCmdBindPipeline(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetReflectionProbePipeline());

        vkCmdBindDescriptorSets(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetReflectionProbePipelineLayout(), 0, 1, &rFrame.frameDescriptorSet, 0, nullptr);

        // -------------------------------------------------------------------------------------------------------------------------
        // Probe camera
        // -------------------------------------------------------------------------------------------------------------------------

        constexpr float c_pi = 3.14159265358979323846f;

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

        const float nearPlane = 0.05f;
        const float farPlane = std::max(rProbe.radius, nearPlane + 0.01f);

        const Math::cMatrix4x4f projection = Math::cMatrix4x4f::perspectiveRH(c_pi * 0.5f, 1.0f, nearPlane, farPlane);

        const Math::cVec3f target = rProbe.position + directions[_faceIndex];

        const Math::cMatrix4x4f view = Math::cMatrix4x4f::lookAtRH(rProbe.position, target, upVectors[_faceIndex]);

        sReflectionProbePushConstants pushConstants{};

        pushConstants.viewProjection = view * projection;

        pushConstants.cameraPosition[0] = rProbe.position.x();
        pushConstants.cameraPosition[1] = rProbe.position.y();
        pushConstants.cameraPosition[2] = rProbe.position.z();
        pushConstants.cameraPosition[3] = 1.0f;

        vkCmdPushConstants(pCommandBuffer, m_pPipeline->GetReflectionProbePipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(sReflectionProbePushConstants), &pushConstants);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndReflectionProbeDraw()
    {
        if (m_renderPassType != sRenderPassType::ReflectionProbe)
        {
            throw std::runtime_error("EndReflectionProbeDraw() called without an active reflection probe draw!");
        }

        sVulkanFrame& rFrame = m_frames[m_currentFrame];
        VkCommandBuffer pCommandBuffer = rFrame.pCommandBuffer;

        vkCmdEndRendering(pCommandBuffer);

        m_renderPassType = sRenderPassType::None;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndReflectionProbeRendering()
    {
        if (m_activeReflectionProbeIndex == UINT32_MAX)
        {
            throw std::runtime_error("EndReflectionProbeRendering() called without active reflection probe!");
        }

        if (m_renderPassType != sRenderPassType::None)
        {
            throw std::runtime_error("EndReflectionProbeRendering() called while a reflection probe face is still active!");
        }

        GenerateReflectionProbeCaptureMipmaps();
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
        m_renderPassType = sRenderPassType::None;

        VkImageMemoryBarrier sceneBarrier{};
        sceneBarrier.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        sceneBarrier.oldLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        sceneBarrier.newLayout                          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        sceneBarrier.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        sceneBarrier.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        sceneBarrier.image                              = m_colorBuffer.GetResolveImage();
        sceneBarrier.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        sceneBarrier.subresourceRange.baseMipLevel      = 0;
        sceneBarrier.subresourceRange.levelCount        = 1;
        sceneBarrier.subresourceRange.baseArrayLayer    = 0;
        sceneBarrier.subresourceRange.layerCount        = 1;
        sceneBarrier.srcAccessMask                      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sceneBarrier.dstAccessMask                      = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(_pCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &sceneBarrier);

        DrawBloomPass(_pCommandBuffer);
        DrawCompositePass(_pCommandBuffer, _imageIndex);
        BeginUIDraw(_pCommandBuffer, _imageIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawBloomPass(VkCommandBuffer _pCommandBuffer)
    {
        for (uint32_t level = 0; level < c_bloomLevelCount; ++level)
        {
            DrawBloomLevel(_pCommandBuffer, m_bloomDownsampleImages[level], m_bloomDescriptorSets[level], level == 0 ? 0u : 1u);
        }

        for (uint32_t level = c_bloomLevelCount - 1; level > 0; --level)
        {
            DrawBloomLevel(_pCommandBuffer, m_bloomUpsampleImages[level - 1], m_bloomDescriptorSets[c_bloomLevelCount + level - 1], 2u);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawBloomLevel(
        VkCommandBuffer _pCommandBuffer, cVulkanImage& _rTarget, VkDescriptorSet _descriptorSet, uint32_t _mode)
    {
        VkImageMemoryBarrier bloomBarrier{};
        bloomBarrier.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bloomBarrier.oldLayout                          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        bloomBarrier.newLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        bloomBarrier.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        bloomBarrier.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        bloomBarrier.image                              = _rTarget.GetImage();
        bloomBarrier.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        bloomBarrier.subresourceRange.baseMipLevel      = 0;
        bloomBarrier.subresourceRange.levelCount        = 1;
        bloomBarrier.subresourceRange.baseArrayLayer    = 0;
        bloomBarrier.subresourceRange.layerCount        = 1;
        bloomBarrier.srcAccessMask                      = VK_ACCESS_SHADER_READ_BIT;
        bloomBarrier.dstAccessMask                      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(_pCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &bloomBarrier);

        VkClearValue clearValue{};

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = _rTarget.GetImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue  = clearValue;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.extent    = { _rTarget.GetWidth(), _rTarget.GetHeight() };
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments    = &colorAttachment;

        vkCmdBeginRendering(_pCommandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.width    = static_cast<float>(_rTarget.GetWidth());
        viewport.height   = static_cast<float>(_rTarget.GetHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_pCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = { _rTarget.GetWidth(), _rTarget.GetHeight() };
        vkCmdSetScissor(_pCommandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetBloomPipeline());
        vkCmdBindDescriptorSets(_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPostProcessPipelineLayout(), 0, 1, &_descriptorSet, 0, nullptr);
        vkCmdPushConstants(_pCommandBuffer, m_pPipeline->GetPostProcessPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(_mode), &_mode);
        vkCmdDraw(_pCommandBuffer, 3, 1, 0, 0);
        vkCmdEndRendering(_pCommandBuffer);

        bloomBarrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        bloomBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        bloomBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        bloomBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(_pCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bloomBarrier);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::DrawCompositePass(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex)
    {
        VkImageMemoryBarrier swapchainBarrier{};
        swapchainBarrier.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapchainBarrier.oldLayout                          = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchainBarrier.newLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        swapchainBarrier.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.image                              = m_pSwapchain->GetImages()[_imageIndex];
        swapchainBarrier.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchainBarrier.subresourceRange.baseMipLevel      = 0;
        swapchainBarrier.subresourceRange.levelCount        = 1;
        swapchainBarrier.subresourceRange.baseArrayLayer    = 0;
        swapchainBarrier.subresourceRange.layerCount        = 1;
        swapchainBarrier.dstAccessMask                      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Chain the transition to the image-available semaphore's wait stage.
        vkCmdPipelineBarrier(_pCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapchainBarrier);

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = m_pSwapchain->GetImageViews()[_imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.extent    = m_pSwapchain->GetExtent();
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments    = &colorAttachment;

        vkCmdBeginRendering(_pCommandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.width    = static_cast<float>(m_pSwapchain->GetExtent().width);
        viewport.height   = static_cast<float>(m_pSwapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_pCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = m_pSwapchain->GetExtent();
        vkCmdSetScissor(_pCommandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetCompositePipeline());
        vkCmdBindDescriptorSets(_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline->GetPostProcessPipelineLayout(), 0, 1, &m_postProcessDescriptorSet, 0, nullptr);
        vkCmdDraw(_pCommandBuffer, 3, 1, 0, 0);
        vkCmdEndRendering(_pCommandBuffer);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::BeginUIDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex)
    {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = m_pSwapchain->GetImageViews()[_imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.extent    = m_pSwapchain->GetExtent();
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments    = &colorAttachment;

        vkCmdBeginRendering(_pCommandBuffer, &renderingInfo);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::EndUIDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex)
    {
        vkCmdEndRendering(_pCommandBuffer);

        const sVulkanFrame& rFrame = m_frames[m_currentFrame];
        if (rFrame.timestampQueryPool != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(_pCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, rFrame.timestampQueryPool, 3);
        }

        VkImageMemoryBarrier barrierToPresent{};

        barrierToPresent.sType                              = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToPresent.oldLayout                          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrierToPresent.newLayout                          = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrierToPresent.srcQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToPresent.dstQueueFamilyIndex                = VK_QUEUE_FAMILY_IGNORED;
        barrierToPresent.image                              = m_pSwapchain->GetImages()[_imageIndex];
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


        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_pDevice->GetPhysicalDevice(), &properties);
        m_timestampPeriod = properties.limits.timestampPeriod;

        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDevice->GetPhysicalDevice(), &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDevice->GetPhysicalDevice(), &queueCount, queueProperties.data());
        const uint32_t timestampBits = queueProperties[m_pDevice->GetQueueFamilyIndices().graphicsFamily].timestampValidBits;
        m_timestampMask = timestampBits == 64 ? UINT64_MAX : (uint64_t{ 1 } << timestampBits) - 1;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // fence starts signaled
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (sVulkanFrame& rFrame : m_frames)
        {
            if (m_timestampMask != 0)
            {
                VkQueryPoolCreateInfo queryInfo{};
                queryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
                queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
                queryInfo.queryCount = 6;

                if (vkCreateQueryPool(device, &queryInfo, nullptr, &rFrame.timestampQueryPool) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create GPU timestamp query pool!");
                }
            }

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
            const VkDeviceSize healthBarBufferSize = sizeof(sHealthBarData) * c_maxNumberOfHealthBars;
            rFrame.healthBarBuffer.Create(*m_pDevice, healthBarBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.healthBarBuffer.Map(*m_pDevice);

            const VkDeviceSize particleBufferSize = sizeof(sParticleData) * c_maxParticleDraws;
            rFrame.particleBuffer.Create(*m_pDevice, particleBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.particleBuffer.Map(*m_pDevice);

            rFrame.instanceBuffer.Create(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.instanceBufferStaging.Create(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.instanceBufferStaging.Map(*m_pDevice, sizeof(sInstanceData) * c_maxNumberOfInstances, 0);

            // light
            rFrame.lightBuffer.Create(*m_pDevice, sizeof(sLightGPU) * c_maxNumberOfLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.lightStagingBuffer.Create(*m_pDevice, sizeof(sLightGPU) * c_maxNumberOfLights, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.lightStagingBuffer.Map(*m_pDevice, sizeof(sLightGPU) * c_maxNumberOfLights, 0);

            rFrame.activeLightIndexBuffer.Create(*m_pDevice, sizeof(uint32_t) * c_maxNumberOfActiveLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            rFrame.activeLightIndexStagingBuffer.Create(*m_pDevice, sizeof(uint32_t) * c_maxNumberOfActiveLights, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rFrame.activeLightIndexStagingBuffer.Map(*m_pDevice, sizeof(uint32_t) * c_maxNumberOfActiveLights, 0);
            
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

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = c_maxNumberOfFrames;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = c_maxNumberOfFrames * 5;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[2].descriptorCount = c_maxNumberOfFrames * (7 + c_maxNumberOfActiveReflectionProbes) + c_maxNumberOfReflectionProbes + 2 * (1 + c_bloomPassCount);

        poolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[3].descriptorCount = c_maxNumberOfFrames * 3 + c_maxNumberOfReflectionProbes + 2 * (1 + c_bloomPassCount);

        VkDescriptorPoolCreateInfo poolInfo{};

        poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount  = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes     = poolSizes.data();
        poolInfo.maxSets        = c_maxNumberOfFrames + c_maxNumberOfReflectionProbes + 1 + c_bloomPassCount;

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

            VkDescriptorBufferInfo activeLightIndexBufferInfo{};

            activeLightIndexBufferInfo.buffer = m_frames[index].activeLightIndexBuffer.GetBuffer();
            activeLightIndexBufferInfo.offset = 0;
            activeLightIndexBufferInfo.range  = sizeof(uint32_t) * c_maxNumberOfActiveLights;

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

            std::array<VkDescriptorImageInfo, c_maxNumberOfActiveReflectionProbes> reflectionProbeImageInfos{};

            for (uint32_t probeIndex = 0; probeIndex < c_maxNumberOfActiveReflectionProbes; ++probeIndex)
            {
                reflectionProbeImageInfos[probeIndex].sampler     = VK_NULL_HANDLE;
                reflectionProbeImageInfos[probeIndex].imageView   = m_environment.GetImageView();
                reflectionProbeImageInfos[probeIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            std::array<VkWriteDescriptorSet, 14> descriptorWrites{};

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

            descriptorWrites[12].sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[12].dstSet             = m_frames[index].frameDescriptorSet;
            descriptorWrites[12].dstBinding         = 12;
            descriptorWrites[12].dstArrayElement    = 0;
            descriptorWrites[12].descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[12].descriptorCount    = c_maxNumberOfActiveReflectionProbes;
            descriptorWrites[12].pImageInfo         = reflectionProbeImageInfos.data();

            descriptorWrites[13].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[13].dstSet          = m_frames[index].frameDescriptorSet;
            descriptorWrites[13].dstBinding      = 13;
            descriptorWrites[13].dstArrayElement = 0;
            descriptorWrites[13].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[13].descriptorCount = 1;
            descriptorWrites[13].pBufferInfo     = &activeLightIndexBufferInfo;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanRenderer::CreateMaterialBuffer()
    {
        
        m_materialBuffer.Create(*m_pDevice, sizeof(sMaterial) * c_maxNumberOfMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        for (cVulkanBuffer& rBuffer : m_materialStagingBuffers)
        {
            rBuffer.Create(*m_pDevice, sizeof(sMaterial) * c_maxNumberOfMaterials, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            rBuffer.Map(*m_pDevice, sizeof(sMaterial) * c_maxNumberOfMaterials, 0);
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

        const Math::cVec3f cameraPosition =
        {
            frameData.cameraPosition[0],
            frameData.cameraPosition[1],
            frameData.cameraPosition[2]
        };

        _rCamera.GetDirection(frameData.cameraDirection);

        frameData.viewportSize[0] = width;
        frameData.viewportSize[1] = height;
        frameData.viewportSize[2] = width  > 0.0f ? 1.0f / width  : 0.0f;
        frameData.viewportSize[3] = height > 0.0f ? 1.0f / height : 0.0f;

        frameData.clipPlanes[0] = _rCamera.GetNearPlane();
        frameData.clipPlanes[1] = _rCamera.GetFarPlane();
        frameData.clipPlanes[2] = 0.0f;
        frameData.clipPlanes[3] = 0.0f;

        frameData.lightCount       = static_cast<uint32_t>(std::min(LightManager::GetLights().size(), static_cast<size_t>(c_maxNumberOfLights)));
        frameData.materialCount    = static_cast<uint32_t>(MaterialManager::GetMaterials().size());
        frameData.activeLightCount = static_cast<uint32_t>(m_activeLightIndices.size());

        m_activeReflectionProbeHandles = ReflectionProbeManager::FindActiveProbeIndices(cameraPosition, c_maxNumberOfActiveReflectionProbes, 1);
        m_visibleReflectionProbeHandles.clear();

        for (ReflectionProbeHandle probeHandle : m_activeReflectionProbeHandles)
        {
            if (!ReflectionProbeManager::GetProbe(probeHandle).dirty)
            {
                m_visibleReflectionProbeHandles.push_back(probeHandle);
            }
        }

        UpdateReflectionProbeDescriptors(_rFrame, m_visibleReflectionProbeHandles);
        frameData.reflectionProbeCount = static_cast<uint32_t>(m_visibleReflectionProbeHandles.size());

        for (uint32_t slotIndex = 0; slotIndex < frameData.reflectionProbeCount; ++slotIndex)
        {
            const ReflectionProbeHandle probeHandle = m_visibleReflectionProbeHandles[slotIndex];

            const sReflectionProbe&       rProbe        = ReflectionProbeManager::GetProbe(probeHandle);
            const cVulkanReflectionProbe& rVulkanProbe  = *m_vulkanReflectionProbes[probeHandle];

            sReflectionProbeGPU& rProbeGPU = frameData.reflectionProbes[slotIndex];

            rProbeGPU.positionMaxMip[0] = rProbe.position.x();
            rProbeGPU.positionMaxMip[1] = rProbe.position.y();
            rProbeGPU.positionMaxMip[2] = rProbe.position.z();
            rProbeGPU.positionMaxMip[3] = static_cast<float>(rVulkanProbe.GetMipLevels() - 1);

            rProbeGPU.boxMinBlendDistance[0] = rProbe.boxMin.x();
            rProbeGPU.boxMinBlendDistance[1] = rProbe.boxMin.y();
            rProbeGPU.boxMinBlendDistance[2] = rProbe.boxMin.z();
            rProbeGPU.boxMinBlendDistance[3] = rProbe.blendDistance;

            rProbeGPU.boxMax[0] = rProbe.boxMax.x();
            rProbeGPU.boxMax[1] = rProbe.boxMax.y();
            rProbeGPU.boxMax[2] = rProbe.boxMax.z();
            rProbeGPU.boxMax[3] = static_cast<float>(rProbe.projectionType);
        }

        _rFrame.frameUniformedBuffer.Write(&frameData, sizeof(sFrameUniformData), 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
