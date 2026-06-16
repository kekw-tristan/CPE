#pragma once 

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Engine::GFX
{
    class cVulkanDevice;
    class cVulkanSwapchain; 

    class cVulkanPipeline
    {
        public:

            cVulkanPipeline() = default;
           ~cVulkanPipeline() = default;

            cVulkanPipeline(const cVulkanPipeline&)         = delete; 
            cVulkanDevice& operator=(const cVulkanDevice&)  = delete;

        public:

            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapchain); 
            void Shutdown(cVulkanDevice& _rDevice);

        public:

            VkPipeline              GetPipeline(); 
            VkPipelineLayout        GetPipelineLayout();
            VkDescriptorSetLayout   GetFrameUniformDescriptorSetLayout();

        private:

            static std::vector<char> ReadFile(const std::string& _rFileName);
            VkShaderModule CreateShaderModule(cVulkanDevice& _rDevice, const std::vector<char>& _rCode); 
            void CreateFrameUniformDescriptorSetLayout(cVulkanDevice& _rDevice);

        private:

            VkPipelineLayout m_pPipelineLayout;
            VkPipeline       m_pGraphicsPipeline; 

            VkDescriptorSetLayout m_pFrameUniformDescriptorSetLayout;

    };
}