#pragma once 

#include <vulkan/vulkan.h>

namespace Engine::Platform
{
    class cWindow;
}

namespace Engine::GFX
{
    class cVulkanContext
    {
        public:

            cVulkanContext() = default;
           ~cVulkanContext() = default;

           cVulkanContext(const cVulkanContext&)            = delete;
           cVulkanContext& operator=(const cVulkanContext&) = delete;

        public:

           void Init(const Platform::cWindow& _pWindow); 
           void Shutdown();

        public:

            VkInstance   GetInstance() const;
            VkSurfaceKHR GetSurface()  const; 

        private:

            void CreateInstance(); 
            void CreateDebugMessenger();
            void CreateSurface(const Platform::cWindow& _pWindow);

            bool CheckValidationLayerSupport() const;

        private:

            VkInstance                  m_pInstance; 
            VkDebugUtilsMessengerEXT    m_pDebugMessenger;
            VkSurfaceKHR                m_pSurface;
    };
}