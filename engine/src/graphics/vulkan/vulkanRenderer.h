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
        
            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanPipeline& _rPipeline);  
            void ShutDown();  
            void RecreateDepthBuffer();
            void RecreateColorBuffer();

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

        public:

            bool NeedsReflectionProbeUpdate(uint32_t _probeIndex) const;

            void BeginReflectionProbeRendering(uint32_t _probeIndex);
            void BeginReflectionProbeDraw(uint32_t _faceIndex);
            void EndReflectionProbeDraw();
            void EndReflectionProbeRendering();

        public:

            VkDescriptorPool GetImguiDescriptorPool();

        public:

            uint32_t GetShadowCount() const;
            uint32_t GetShadowMatrixCount(uint32_t _shadowIndex) const;

        private:

            void EndDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex);

        private:

            void CreateFrameResources();
            void CreateRenderFinishedSemaphores(); 
            void CreateDescriptorPool();
            void CreateImGuiDescriptorPool();
            void CreateDescriptorSets();
            void CreateMaterialBuffer();

            void UpdateFrameUniformBuffer(sVulkanFrame& _rFrame, const cCamera& _rCamera);
            void UpdateLightBuffer();
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

            void CreateReflectionProbePrefilterDescriptorSets();
            void GenerateReflectionProbeCaptureMipmaps();

        private:

            cVulkanDevice*      m_pDevice;
            cVulkanSwapchain*   m_pSwapchain;
            cVulkanCommands*    m_pCommands;
            cVulkanPipeline*    m_pPipeline;

        private:

            std::array<sVulkanFrame, c_maxNumberOfFrames> m_frames;
            int m_currentFrame; 

            VkDescriptorPool m_pDescriptorPool;
            VkDescriptorPool m_pImGuiDescriptorPool;

            cVulkanDepthBuffer m_depthBuffer;
            cVulkanColorBuffer m_colorBuffer;
            
            std::vector<const cVulkanMesh*> m_submittedMeshes;
            std::vector<VkSemaphore> m_renderFinishedSemaphores;  
            std::vector<VkFence> m_imagesInFlight;  

            bool m_hasFrameStarted; 
            uint32_t m_imageIndex;

            cVulkanBuffer m_materialBuffer;
            cVulkanBuffer m_materialStagingBuffer;

            cShadowMap m_shadowMap;
            std::vector<int32_t> m_lightShadowIndices; 
            sRenderPassType::Enum m_renderPassType = sRenderPassType::None;

            std::vector<sShadowDataGPU> m_shadowData;
            VkImageLayout m_shadowMapLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            cVulkanEnvironment m_environment;
            cVulkanBRDFLUT m_brdfLUT;

        private:


            cVulkanImage m_reflectionProbeDepthImage;

            VkImageLayout m_reflectionProbeDepthLayout          = VK_IMAGE_LAYOUT_UNDEFINED;

            uint32_t m_reflectionProbeCount = 0;
            uint32_t m_activeReflectionProbeIndex = UINT32_MAX;

            std::vector<std::unique_ptr<cVulkanReflectionProbe>> m_vulkanReflectionProbes;

            std::vector<VkImageLayout> m_reflectionProbeCaptureLayouts;
            std::vector<VkImageLayout> m_reflectionProbePrefilteredLayouts;

            std::vector<VkDescriptorSet> m_reflectionProbePrefilterDescriptorSets;

            bool m_reflectionProbeDirty = true;
    };
}  