#pragma once

#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;
    class cVulkanSwapchain;
    class cVulkanCommands;
    class cVulkanSync;

    class cVulkanRenderer
    {
        public:
            cVulkanRenderer()  = default;
            ~cVulkanRenderer() = default;
        
            cVulkanRenderer(const cVulkanRenderer&)             = delete;
            cVulkanRenderer& operator=(const cVulkanRenderer&)  = delete;

        public:
        
            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapChain, cVulkanCommands& _rCommands, cVulkanSync& _rSync);    
            void DrawFrame();
            
        private:

            void RecordCommandBuffer(uint32_t _imageIndex);
            
        private:

            cVulkanDevice*      m_pDevice;
            cVulkanSwapchain*   m_pSwapchain;
            cVulkanCommands*    m_pCommands;
            cVulkanSync*        m_pSync;
    };
}  