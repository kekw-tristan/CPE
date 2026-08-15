#include "imguiManager.h"

#include <imgui.h>
#include <iostream>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        class cImguiManager
        {

            public:

                static cImguiManager& GetInstance();

            public:

                void Init(const sImGuiInitDesc& _rDesc);
                void Shutdown();

                void BeginFrame();
                void EndFrame(VkCommandBuffer _commandBuffer);

            private:

                cImguiManager(); 
               ~cImguiManager(); 

               cImguiManager(const cImguiManager&)              = delete; 
               cImguiManager& operator=(const cImguiManager&)   = delete;

        };
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cImguiManager& cImguiManager::GetInstance()
        {
            static cImguiManager s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cImguiManager::Init(const sImGuiInitDesc& _rDesc)
        {
            IMGUI_CHECKVERSION();

            ImGui::CreateContext();
            ImGui::StyleColorsDark();

            ImGui_ImplGlfw_InitForVulkan(_rDesc.pWindow, true);

            ImGui_ImplVulkan_InitInfo initInfo{};

            initInfo.ApiVersion     = VK_API_VERSION_1_3;
            initInfo.Instance       = _rDesc.instance;
            initInfo.PhysicalDevice = _rDesc.physicalDevice;
            initInfo.Device         = _rDesc.device;

            initInfo.QueueFamily    = _rDesc.graphicsQueueFamily;
            initInfo.Queue          = _rDesc.graphicsQueue;

            initInfo.DescriptorPool = _rDesc.descriptorPool;

            initInfo.MinImageCount  = _rDesc.minImageCount;
            initInfo.ImageCount     = _rDesc.imageCount;

            initInfo.UseDynamicRendering = true;

            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;

            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_rDesc.colorFormat;

            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = _rDesc.depthFormat;

            initInfo.PipelineInfoMain.MSAASamples = _rDesc.msaaSamples;

            ImGui_ImplVulkan_Init(&initInfo);
        }
        
        // -------------------------------------------------------------------------------------------------------------------------

        void cImguiManager::Shutdown()
        {
            ImGui_ImplVulkan_Shutdown();

            ImGui_ImplGlfw_Shutdown();

            ImGui::DestroyContext();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cImguiManager::BeginFrame()
        {
            ImGui_ImplVulkan_NewFrame();

            ImGui_ImplGlfw_NewFrame();

            ImGui::NewFrame();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cImguiManager::EndFrame(VkCommandBuffer _commandBuffer)
        {
            ImGui::Render();

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), _commandBuffer);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cImguiManager::cImguiManager()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cImguiManager::~cImguiManager()
        {

        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ImGuiManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void Init(const sImGuiInitDesc& _rDesc)
        {
            cImguiManager::GetInstance().Init(_rDesc);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void Shutdown()
        {
            cImguiManager::GetInstance().Shutdown(); 
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void BeginFrame()
        {
            cImguiManager::GetInstance().BeginFrame();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void EndFrame(VkCommandBuffer _commandBuffer)
        {
            cImguiManager::GetInstance().EndFrame(_commandBuffer);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------