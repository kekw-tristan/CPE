#include "graphics/vulkan/vulkanContext.h"

#include "platform/window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------
    // const expressions

    static constexpr const char* c_validationLayerName = "VK_LAYER_KHRONOS_validation";

    // -------------------------------------------------------------------------------------------------------------------------
    // vulkan debug callback

    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      _messageServerity,
        VkDebugUtilsMessageTypeFlagsEXT             _messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData,
        void*                                       _pUserData)
    {
        std::cerr << "[Vulkan Validation] " << _pCallbackData->pMessage << std::endl;
        
        return VK_FALSE;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance                                  _pInstance,
        const VkDebugUtilsMessengerCreateInfoEXT*   _pCreateInfo,
        const VkAllocationCallbacks*                _pAllocator,
        VkDebugUtilsMessengerEXT*                   _pDebugMessenger)
    {
        auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_pInstance, "vkCreateDebugUtilsMessengerEXT"));
        
        if (function)
        {
            return function(_pInstance, _pCreateInfo, _pAllocator, _pDebugMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    static void DestroyDebugUtilsMessengerEXT(
        VkInstance                      _pInstance,
        VkDebugUtilsMessengerEXT        _debugMessenger,
        const VkAllocationCallbacks*    _pAllocator)
    {
        auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_pInstance, "vkDestroyDebugUtilsMessengerEXT"));

        if (function)
        {
            function(_pInstance, _debugMessenger, _pAllocator);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanContext::Init(const Platform::cWindow& _pWindow)
    {
        CreateInstance(); 
        CreateDebugMessenger();

        std::cout << "vulkan context initialized." << std::endl;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanContext::Shutdown()
    {
        if (m_pDebugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_pInstance, m_pDebugMessenger, nullptr);
            m_pDebugMessenger = VK_NULL_HANDLE;
        }

        if (m_pInstance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_pInstance, nullptr);
            m_pInstance = VK_NULL_HANDLE;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanContext::CreateInstance()
    {
        if (!CheckValidationLayerSupport())
        {
            throw std::runtime_error("Vulkan validation layer not available!");
        }

        VkApplicationInfo appInfo{}; 

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName    = "game";
        appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName         = "engine";
        appInfo.engineVersion       = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion          = VK_API_VERSION_1_3;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        if (!glfwExtensions)
        {
            throw std::runtime_error("Failed to get required GLFW Vulkan extensions!"); 
        }

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        std::vector<const char*> validationLayers = { c_validationLayerName };

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VkInstanceCreateInfo createInfo{};

        createInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo         = &appInfo;
        createInfo.enabledExtensionCount    = static_cast<uint32_t>(extensions.size()); 
        createInfo.ppEnabledExtensionNames  = extensions.data();
        createInfo.enabledLayerCount        = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames      = validationLayers.data(); 

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_pInstance); 

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan instance");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanContext::CreateDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};

        createInfo.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback  = VulkanDebugCallback;
        createInfo.pUserData        = nullptr;

        VkResult result = CreateDebugUtilsMessengerEXT(m_pInstance, &createInfo, nullptr, &m_pDebugMessenger);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan debug messenger."); 
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cVulkanContext::CheckValidationLayerSupport() const
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const VkLayerProperties& layerProperies : availableLayers)
        {
            if (std::strcmp(layerProperies.layerName, c_validationLayerName) == 0)
            {
                return true;
            }
        }

        return false; 
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------