#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Engine::GFX
{
    struct sImGuiInitDesc
    {
        GLFWwindow* pWindow = nullptr;

        VkInstance       instance       = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice         device         = VK_NULL_HANDLE;

        VkQueue  graphicsQueue          = VK_NULL_HANDLE;
        uint32_t graphicsQueueFamily    = 0;

        VkRenderPass     renderPass     = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        uint32_t imageCount     = 0;
        uint32_t minImageCount  = 0;

        VkFormat colorFormat;
        VkFormat depthFormat;
        VkSampleCountFlagBits msaaSamples;
    };

    namespace ImGuiManager
    {
        void Init(const sImGuiInitDesc& _rDesc);
        void Shutdown();


        void BeginFrame();
        void EndFrame(VkCommandBuffer _commandBuffer);
    }
    
}