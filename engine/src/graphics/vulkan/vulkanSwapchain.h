#pragma once 

#include <vulkan/vulkan.h>

#include <vector> 

namespace Engine::Platform
{
    class cWindow;
}

namespace Engine::GFX
{
    class cVulkanContext;
    class cVulkanDevice; 

    struct sSwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    class  cVulkanSwapchain
    {
        public:

            cVulkanSwapchain() = default;
           ~cVulkanSwapchain() = default;

            cVulkanSwapchain(const cVulkanSwapchain&)               = delete;
            cVulkanSwapchain& operator=(const cVulkanSwapchain&)    = delete;

        public:

            void Init(const cVulkanContext& _rVulkanContext, const cVulkanDevice& _rDevice, const Platform::cWindow& _rWindow);
            void Shutdown(const cVulkanDevice& _rDevice);
            void Recreate(cVulkanContext& _rContext, cVulkanDevice& _rDevice, Engine::Platform::cWindow& _rWindow);

        public:

            VkSwapchainKHR  GetSwapchain()   const;
            VkFormat        GetImageFormat() const;
            VkExtent2D      GetExtent()      const;

            const std::vector<VkImage>&     GetImages()     const; 
            const std::vector<VkImageView>& GetImageViews() const; 

        private:

            void CreateSwapchain(const cVulkanContext& _rContext, const cVulkanDevice& _rDevice, const Platform::cWindow& _rWindow);
            void CreateImageViews(const cVulkanDevice& _rDevice); 
            
            sSwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice _pPhysicalDevice, VkSurfaceKHR _pSurface)                const;
            VkSurfaceFormatKHR       ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _rAvailableFormats)                  const;
            VkPresentModeKHR         ChoosePresentMode(const std::vector<VkPresentModeKHR>& _rAvailablePresentModes)                 const; 
            VkExtent2D               ChooseExtent(const VkSurfaceCapabilitiesKHR& _rCapabilities, const Platform::cWindow& _rWindow) const;

        private:
    
            VkSwapchainKHR m_pSwapchain; 

            std::vector<VkImage>     m_images;
            std::vector<VkImageView> m_imageViews;

            VkFormat   m_imageFormat; 
            VkExtent2D m_extent;

    };
}