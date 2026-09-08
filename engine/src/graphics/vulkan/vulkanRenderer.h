#pragma once

#include "graphics/gfxConfig.h"
#include "graphics/healthBarData.h"

#include <span>

#include "graphics/vulkan/shadowData.h"
#include "graphics/vulkan/shadowMap.h"
#include "graphics/vulkan/vulkanColorBuffer.h"
#include "graphics/vulkan/vulkanEnvironment.h"
#include "graphics/vulkan/vulkanFrame.h"
#include "graphics/vulkan/vulkanDepthBuffer.h"
#include "graphics/vulkan/vulkanBRDFLUT.h"
#include "graphics/vulkan/reflectionProbe.h"
#include "graphics/vulkan/vulkanReflectionProbe.h"

#include "graphics/reflectionProbes/reflectionProbeManager.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <utility>

namespace Engine::GFX
{
    struct sInstanceData;

    class cCamera;
    class cVulkanDevice;
    class cVulkanSwapchain;
    class cVulkanCommands;
    class cVulkanSync;
    class cVulkanPipeline;
    class cVulkanMesh;

    struct sRenderPassType
    {
        enum Enum
        {
            None,
            Shadow,
            ReflectionProbe,
            AmbientOcclusion,
            Main
        };
    };

    class cVulkanRenderer
    {
        public:

            cVulkanRenderer()  = default;
            ~cVulkanRenderer() = default;
        
            cVulkanRenderer(const cVulkanRenderer&)             = delete;
            cVulkanRenderer& operator=(const cVulkanRenderer&)  = delete;

        public:
        
            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanPipeline& _rPipeline, const sEnvironmentSettings& _rEnvironment = {});
            void ShutDown();  
            void RecreateDepthBuffer();
            void RecreateColorBuffer();
            void SetBackgroundColor(const std::array<float, 4>& _rColor);

        public:

            void SubmitMesh(const cVulkanMesh& _rMesh); 
            void ClearSubmittedMeshes();

        public:

            bool BeginFrame(const cCamera& _rCamera);
            bool EndFrame();
            void DrawMeshIntances(cVulkanMesh* _pMesh, uint32_t _instanceCount, uint32_t _firstInstance = 0);
            void UpdateInstanceBuffer(std::vector<sInstanceData*>& _rInstances); 
            void UpdateHealthBars(std::span<const sHealthBarData> _healthBars);
            void DrawHealthBars();

            void BeginDraw(); 
            void BeginAmbientOcclusionDraw();
            void EndAmbientOcclusionDraw();

        public:

            bool NeedsReflectionProbeUpdate(uint32_t _probeIndex) const;

            void BeginReflectionProbeRendering(uint32_t _probeIndex);
            void BeginReflectionProbeDraw(uint32_t _faceIndex);
            void EndReflectionProbeDraw();
            void EndReflectionProbeRendering();

        public:

            VkDescriptorPool GetImguiDescriptorPool();

        public:

            // Negative values mean no completed GPU measurement is available.
            const std::array<double, 2>& GetGpuPassMilliseconds() const { return m_gpuPassMilliseconds; }
            uint32_t GetShadowCount() const;
            uint32_t GetShadowMatrixCount(uint32_t _shadowIndex) const;

        private:

            void EndDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex);
            void EndUIDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex);
            void CreateBloomBuffer();
            void DestroyBloomBuffer();
            void CreatePostProcessSampler();
            void UpdatePostProcessDescriptorSet();
            void DrawBloomPass(VkCommandBuffer _pCommandBuffer);
            void DrawBloomLevel(VkCommandBuffer _pCommandBuffer, cVulkanImage& _rTarget, VkDescriptorSet _descriptorSet, uint32_t _mode);
            void DrawCompositePass(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex);
            void BeginUIDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex);
            void CreateAmbientOcclusionBuffers();
            void DestroyAmbientOcclusionBuffers();
            void UpdateAmbientOcclusionDescriptors();
            void TransitionAmbientOcclusionImage(cVulkanImage& _rImage, bool _renderTarget);
            void DrawAmbientOcclusionFilter(cVulkanImage& _rTarget, VkPipeline _pipeline);

        private:

            void CreateFrameResources();
            void CreateRenderFinishedSemaphores(); 
            void CreateDescriptorPool();
            void CreateImGuiDescriptorPool();
            void CreateDescriptorSets();
            void CreatePostProcessDescriptorSet();
            void CreateMaterialBuffer();

            void UpdateFrameUniformBuffer(sVulkanFrame& _rFrame, const cCamera& _rCamera);
            void UpdateLightBuffer();
            void SelectActiveLights(const cCamera& _rCamera);
            void UpdateActiveLightIndexBuffer();
            void UpdateMaterialBuffer();
            void UpdateShadowBuffer(const cCamera& _rCamera);
            void UpdateReflectionProbeDescriptors(sVulkanFrame& _rFrame, const std::vector<ReflectionProbeHandle>& _rActiveProbeHandles);

        public:

            void BeginShadowRendering();
            void EndShadowRendering();

            void BeginShadowDraw(uint32_t _shadowIndex, uint32_t _matrixIndex);
            void DrawShadowMeshInstances(cVulkanMesh* _pMesh, uint32_t _instanceCount, uint32_t _firstInstance);
            void EndShadowDraw();

            void PrefilterReflectionProbe(uint32_t _probeIndex);
            uint32_t GetReflectionProbeCount() const;

        private:

            void CreateReflectionProbePrefilterDescriptorSets(uint32_t _firstProbeIndex = 0);
            void UpdateReflectionProbePrefilterDescriptorSet(uint32_t _probeIndex);
            void GenerateReflectionProbeCaptureMipmaps();
            void EnsureReflectionProbeResources();

        private:

            cVulkanDevice*      m_pDevice;
            cVulkanSwapchain*   m_pSwapchain;
            cVulkanCommands*    m_pCommands;
            cVulkanPipeline*    m_pPipeline;

        private:

            std::array<sVulkanFrame, c_maxNumberOfFrames> m_frames;
            std::array<float, 4> m_backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            int m_currentFrame; 

            VkDescriptorPool m_pDescriptorPool;
            VkDescriptorPool m_pImGuiDescriptorPool;

            cVulkanDepthBuffer m_depthBuffer;
            cVulkanColorBuffer m_colorBuffer;

            cVulkanImage m_occlusionGeometry;
            cVulkanImage m_occlusionDepth;
            cVulkanImage m_occlusionRaw;
            cVulkanImage m_occlusionFiltered;

            static constexpr uint32_t c_bloomLevelCount = 6;
            static constexpr uint32_t c_bloomPassCount  = c_bloomLevelCount * 2 - 1;

            std::array<cVulkanImage, c_bloomLevelCount> m_bloomDownsampleImages;
            std::array<cVulkanImage, c_bloomLevelCount - 1> m_bloomUpsampleImages;
            std::array<VkDescriptorSet, c_bloomPassCount> m_bloomDescriptorSets{};

            VkSampler       m_postProcessSampler        = VK_NULL_HANDLE;
            VkDescriptorSet m_postProcessDescriptorSet  = VK_NULL_HANDLE;
            
            std::vector<const cVulkanMesh*> m_submittedMeshes;
            std::vector<VkSemaphore>        m_renderFinishedSemaphores;  
            std::vector<VkFence>            m_imagesInFlight;  

            bool m_hasFrameStarted; 
            uint32_t m_imageIndex;

            cVulkanBuffer m_materialBuffer;
            std::array<cVulkanBuffer, c_maxNumberOfFrames> m_materialStagingBuffers;

            cShadowMap m_shadowMap
                ;
            std::vector<int32_t>                    m_lightShadowIndices; 
            std::vector<uint32_t>                   m_activeLightIndices;
            std::vector<uint32_t>                   m_previousActiveLightIndices;
            std::vector<std::pair<float, uint32_t>> m_activeLightCandidates;

            sRenderPassType::Enum m_renderPassType = sRenderPassType::None;

            std::array<double, 2> m_gpuPassMilliseconds = { -1.0, -1.0 };
            double m_timestampPeriod = 0.0;
            uint64_t m_timestampMask = 0;
            std::vector<sShadowDataGPU> m_shadowData;
            VkImageLayout m_shadowMapLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            cVulkanEnvironment m_environment;
            cVulkanBRDFLUT m_brdfLUT;

        private:


            cVulkanImage m_reflectionProbeDepthImage;

            VkImageLayout m_reflectionProbeDepthLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
            uint32_t m_reflectionProbeDepthResolution           = 1;

            uint32_t m_reflectionProbeCount = 0;
            uint32_t m_activeReflectionProbeIndex = UINT32_MAX;

            std::vector<std::unique_ptr<cVulkanReflectionProbe>> m_vulkanReflectionProbes;
            std::vector<ReflectionProbeHandle> m_activeReflectionProbeHandles;
            std::vector<ReflectionProbeHandle> m_visibleReflectionProbeHandles;

            std::vector<VkImageLayout> m_reflectionProbeCaptureLayouts;
            std::vector<VkImageLayout> m_reflectionProbePrefilteredLayouts;

            std::vector<VkDescriptorSet> m_reflectionProbePrefilterDescriptorSets;

            bool m_reflectionProbeDirty = true;
    };
}
