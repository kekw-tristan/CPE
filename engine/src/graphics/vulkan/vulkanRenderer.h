#pragma once

#include "graphics/gfxConfig.h"
#include "graphics/vulkan/vulkanFrame.h"
#include "graphics/vulkan/vulkanDepthBuffer.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>


namespace Engine::GFX
{
    class cCamera;
    class cVulkanDevice;
    class cVulkanSwapchain;
    class cVulkanCommands;
    class cVulkanSync;
    class cVulkanPipeline;
    class cVulkanMesh;

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

        public:

            void SubmitMesh(const cVulkanMesh& _rMesh); 
            void ClearSubmittedMeshes();

        public:

            bool BeginFrame(const cCamera& _rCamera);
            bool EndFrame();

        private:

            void BeginDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex, sVulkanFrame& _rFrame); 
            void EndDraw(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex);
            
        private:

            void CreateFrameResources();
            void CreateDescriptorPool();
            void CreateDescriptorSets();
            void UpdateFrameUniformBuffer(sVulkanFrame& _rFrame, const cCamera& _rCamera);
            void DrawSubmittedInstances(VkCommandBuffer _pCommandBuffer);
            
        private:

            cVulkanDevice*      m_pDevice;
            cVulkanSwapchain*   m_pSwapchain;
            cVulkanCommands*    m_pCommands;
            cVulkanPipeline*    m_pPipeline;

        private:

            std::array<sVulkanFrame, c_maxNumberOfFrames> m_frames;
            int m_currentFrame; 

            VkDescriptorPool m_pDescriptorPool;

            cVulkanDepthBuffer m_depthBuffer;

            std::vector<const cVulkanMesh*> m_submittedMeshes;

            bool m_hasFrameStarted; 
            uint32_t m_imageIndex;
    };
}  