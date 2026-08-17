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

            cVulkanPipeline(const cVulkanPipeline&)          = delete; 
            cVulkanPipeline& operator=(const cVulkanDevice&) = delete;

        public:

            void Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapchain); 
            void Shutdown(cVulkanDevice& _rDevice);

        public:

            VkPipeline              GetPipeline(); 
            VkPipelineLayout        GetPipelineLayout();

            VkPipeline              GetShadowPipeline(); 
            VkPipelineLayout        GetShadowPipelineLayout();

            VkPipeline              GetReflectionProbePipeline();
            VkPipelineLayout        GetReflectionProbePipelineLayout();

            VkPipeline              GetReflectionProbePrefilterPipeline();
            VkPipelineLayout        GetReflectionProbePrefilterPipelineLayout();
            VkDescriptorSetLayout   GetReflectionProbePrefilterDescriptorSetLayout();

            VkDescriptorSetLayout   GetFrameUniformDescriptorSetLayout();

        private:

            static std::vector<char> ReadFile(const std::string& _rFileName);
            VkShaderModule CreateShaderModule(cVulkanDevice& _rDevice, const std::vector<char>& _rCode); 
            void CreateFrameUniformDescriptorSetLayout(cVulkanDevice& _rDevice);
            void CreateShadowPipeline(cVulkanDevice& _rDevice);
            void CreateReflectionProbePipeline(cVulkanDevice& _rDevice);
            void CreateReflectionProbePrefilterPipeline(cVulkanDevice& _rDevice);

        private:

            VkPipelineLayout m_pPipelineLayout;
            VkPipeline       m_pGraphicsPipeline; 

            VkPipelineLayout m_pShadowPipelineLayout = VK_NULL_HANDLE;
            VkPipeline       m_pShadowPipeline       = VK_NULL_HANDLE;

            VkPipeline       m_pReflectionProbePipeline       = VK_NULL_HANDLE;
            VkPipelineLayout m_pReflectionProbePipelineLayout = VK_NULL_HANDLE;

            VkPipeline              m_pReflectionProbePrefilterPipeline             = VK_NULL_HANDLE;
            VkPipelineLayout        m_pReflectionProbePrefilterPipelineLayout       = VK_NULL_HANDLE;
            VkDescriptorSetLayout   m_pReflectionProbePrefilterDescriptorSetLayout  = VK_NULL_HANDLE;

            VkDescriptorSetLayout m_pFrameUniformDescriptorSetLayout;

    };
}