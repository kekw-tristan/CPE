#pragma once

#include "graphics/gfxConfig.h"
#include "graphics/vulkan/vulkanFrame.h"

#include <array>
#include <vulkan/vulkan.h>

namespace Engine::GFX
{
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
        
            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanPipeline& _rPipeline, cVulkanMesh& _rMesh);  
            void ShutDown();  
            bool DrawFrame();
            
        private:

            void RecordCommandBuffer(VkCommandBuffer _pCommandBuffer, uint32_t _imageIndex, sVulkanFrame& _rFrame);
            void CreateFrameResources();
            void CreateDescriptorPool();
            void CreateDescriptorSets();
            void UpdateFrameUniformBuffer(sVulkanFrame& _rFrame);
            
        private:

            cVulkanDevice*      m_pDevice;
            cVulkanSwapchain*   m_pSwapchain;
            cVulkanCommands*    m_pCommands;
            cVulkanPipeline*    m_pPipeline;
            cVulkanMesh*        m_pMesh;

        private:

            std::array<sVulkanFrame, c_maxNumberOfFrames> m_frames;
            int m_currentFrame; 

            VkDescriptorPool m_pDescriptorPool;
            
    };
}  