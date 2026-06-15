#pragma once

#include "graphics/vulkan/vulkanCommands.h"
#include "graphics/vulkan/vulkanContext.h"
#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanMesh.h"
#include "graphics/vulkan/vulkanPipeline.h"
#include "graphics/vulkan/vulkanRenderer.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanSync.h"

#include "platform/window.h"

namespace Engine
{
    class cApplication
    {
        public:

            cApplication();
           ~cApplication();

            cApplication(const cApplication&)               = delete;
            cApplication& operator=(const cApplication&)    = delete;

        public:
        
            void Run();

        private:

            Platform::cWindow m_window;

            GFX::cVulkanContext     m_vulkanContext;
            GFX::cVulkanDevice      m_vulkanDevice;
            GFX::cVulkanSwapchain   m_vulkanSwapchain;
            GFX::cVulkanCommands    m_vulkanCommands; 
            GFX::cVulkanSync        m_vulkanSync;
            GFX::cVulkanRenderer    m_vulkanRenderer;
            GFX::cVulkanPipeline    m_vulkanPipeline;
            GFX::cVulkanMesh        m_quadMesh;
    };
}