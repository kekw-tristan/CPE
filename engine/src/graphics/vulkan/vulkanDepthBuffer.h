#include <vulkan/vulkan.h>

namespace Engine::GFX
{
    class cVulkanDevice;
    class cVulkanSwapchain;
    class cVulkanCommands;

    class cVulkanDepthBuffer
    {

        public:

            cVulkanDepthBuffer();
            ~cVulkanDepthBuffer()   = default;

            cVulkanDepthBuffer(const cVulkanDepthBuffer&)               = delete;
            cVulkanDepthBuffer& operator=(const cVulkanDepthBuffer&)    = delete;

        public:

            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapchain, cVulkanCommands& _rCommands);
            void ShutDown(cVulkanDevice& _rDevice);

        public:
        
            VkImageView GetImageView()  const; 
            VkFormat    GetFormat()     const;

        private:

            void CreateImage(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height);
            void CreateImageView(cVulkanDevice& _rDevice);
            void TransitionImageLayout(cVulkanDevice& _rDevice, cVulkanCommands& _rCommands);

        private:

            VkImage         m_pImage; 
            VkDeviceMemory  m_pMemory;
            VkImageView     m_pImageView;
            VkFormat        m_format;
    };
}