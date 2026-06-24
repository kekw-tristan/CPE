#pragma once

#include "core/timer.h"

#include "graphics/camera.h"
#include "graphics/meshCache.h"

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

namespace Engine
{
    struct sAppConfig; 
}

namespace Engine::GFX
{
    using MeshHandle = void*;
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
            void Draw(GFX::MeshHandle _pHandle);

        private:

            void UpdateCamera(float _deltaTime);

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
            GFX::cMeshCache         m_geometryCache;

            std::vector<std::unique_ptr<GFX::cVulkanMesh>> m_vulkanMeshes;
    };
}