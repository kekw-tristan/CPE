#pragma once

#include "graphics/vulkan/vulkanContext.h"

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

            GFX::cVulkanContext m_vulkanContext;
    };
}