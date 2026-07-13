#pragma once

#include "core/timer.h"

#include "graphics/camera.h"

#include "graphics/vulkan/vulkanCommands.h"
#include "graphics/vulkan/vulkanContext.h"
#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanMesh.h"
#include "graphics/vulkan/vulkanPipeline.h"
#include "graphics/vulkan/vulkanRenderer.h"
#include "graphics/vulkan/vulkanSwapchain.h"

#include "platform/input.h"
#include "platform/window.h"

#include <vector>
#include <array>

namespace Engine
{
    struct sAppConfig; 
}

namespace Engine::GFX
{
    using MeshHandle = void*;
    struct sInstanceData;
    struct sMeshData;
}

namespace Engine::Logic
{
    class cApplicationIntern
    {
        public:

            cApplicationIntern(sAppConfig& _rAppConf);
           ~cApplicationIntern();

            cApplicationIntern(const cApplicationIntern&)               = delete;
            cApplicationIntern& operator=(const cApplicationIntern&)    = delete;

        public:

            bool BeginFrame(GFX::cCamera& _rCamera);
            bool EndFrame();
            bool GetShouldClose();
            void Update();
            float GetDeltaTime();
            void RecreateSwapchain();
            GFX::cCamera& GetCamera();


            GFX::MeshHandle CreateMesh(GFX::sMeshData& _rMeshData);
            void SubmitMesh(GFX::MeshHandle _pHandle);
            void DrawMeshIntances(GFX::MeshHandle _pHandle, std::vector<GFX::sInstanceData*>& _rInstances);
            void UpdateInstanceBuffer(std::vector<GFX::sInstanceData*>& _rInstances);
            void BeginDraw(); 
            
        public:

            bool IsKeydown(int _key) const;

        private:

            Platform::cWindow m_window;
            Platform::cInput  m_input;

            Core::cTimer m_Timer;

            GFX::cVulkanContext     m_vulkanContext;
            GFX::cVulkanDevice      m_vulkanDevice;
            GFX::cVulkanSwapchain   m_vulkanSwapchain;
            GFX::cVulkanCommands    m_vulkanCommands; 
            GFX::cVulkanRenderer    m_vulkanRenderer;
            GFX::cVulkanPipeline    m_vulkanPipeline;

            GFX::cCamera            m_camera;

            std::vector<std::unique_ptr<GFX::cVulkanMesh>> m_vulkanMeshes;
    };
}