#include "applicationIntern.h"

#include "application.h"

#include "graphics/vertex.h"

#include "graphics/vulkan/vulkanBuffer.h"
#include "graphics/vulkan/vulkanVertex.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Logic
{

    // -------------------------------------------------------------------------------------------------------------------------
    // application contructor
    // initializes window and vulkan context
    
    cApplicationIntern::cApplicationIntern(sAppConfig& _rAppConf)
        : m_window(1280, 720, "Vulkan Engine")
    {
        m_vulkanContext  .Init(m_window);
        m_vulkanDevice   .Init(m_vulkanContext);
        m_vulkanCommands .Init(m_vulkanDevice);
        m_vulkanSwapchain.Init(m_vulkanContext, m_vulkanDevice, m_window);
        m_vulkanPipeline .Init(m_vulkanDevice, m_vulkanSwapchain);
        m_vulkanRenderer .Init(m_vulkanDevice, m_vulkanSwapchain, m_vulkanCommands, m_vulkanPipeline);

        m_camera.LookAt(
            2.0f, 1.5f, 3.0f,
            0.0f, 0.0f, 0.0f
        );

        m_camera.SetPerspective(
            60.0f,
            0.1f,
            100.0f
        );

        m_Timer.Reset();
        m_input.Init(m_window.GetWindow());
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // application destructor

    cApplicationIntern::~cApplicationIntern()
    {
        m_vulkanDevice.WaitIdle();

        for (const std::unique_ptr<GFX::cVulkanMesh>& pMesh : m_vulkanMeshes)
        {
            pMesh->Shutdown(m_vulkanDevice);
        }
        m_vulkanRenderer .ShutDown();
        m_vulkanPipeline .Shutdown(m_vulkanDevice);
        m_vulkanCommands .Shutdown(m_vulkanDevice);
        m_vulkanSwapchain.Shutdown(m_vulkanDevice);
        m_vulkanDevice   .Shutdown();
        m_vulkanContext  .Shutdown(); 
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::BeginFrame(GFX::cCamera& _rCamera)
    {
        return m_vulkanRenderer.BeginFrame(_rCamera);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::EndFrame()
    {
        return m_vulkanRenderer.EndFrame();        
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::GetShouldClose()
    {
        return m_window.ShouldClose();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::Update()
    {
        m_window.PollEvents();
        m_input.Update();
        m_Timer.Tick();

        const float deltaTime = m_Timer.GetDeltaTime();

        // fullscreen
        if (m_input.WasKeyPressed(GLFW_KEY_F11))
        {
            m_window.ToggleFullscreen();
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cApplicationIntern::GetDeltaTime()
    {
        return m_Timer.GetDeltaTime();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::RecreateSwapchain()
    {
        m_vulkanDevice.WaitIdle();

        m_window.ResetRezisedFlag();
        m_vulkanSwapchain.Recreate(m_vulkanContext, m_vulkanDevice, m_window);
        m_vulkanRenderer.RecreateDepthBuffer();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GFX::cCamera& cApplicationIntern::GetCamera()
    {
        return m_camera;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GFX::MeshHandle cApplicationIntern::CreateMesh(GFX::sMeshData &_rMeshData)
    {
        std::unique_ptr<GFX::cVulkanMesh> pMesh = std::make_unique<GFX::cVulkanMesh>();

        pMesh->Create(m_vulkanDevice, m_vulkanCommands, _rMeshData.vertices, _rMeshData.indices);

        const uint32_t meshIndex = static_cast<uint32_t>(m_vulkanMeshes.size()); 

        m_vulkanMeshes.push_back(std::move(pMesh));

        return static_cast<GFX::MeshHandle>(m_vulkanMeshes.back().get());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::SubmitMesh(GFX::MeshHandle _pHandle)
    {
        m_vulkanRenderer.SubmitMesh(*static_cast<GFX::cVulkanMesh*>(_pHandle));
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::Draw(GFX::MeshHandle _pHandle, std::array<float, 16>& _rWorldMatrix)
    {
        GFX::cVulkanMesh* pVulkanMesh = static_cast<GFX::cVulkanMesh*>(_pHandle);

        m_vulkanRenderer.Draw(pVulkanMesh, _rWorldMatrix);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::DrawMeshIntances(GFX::MeshHandle _pHandle, std::vector<GFX::sInstanceData*>& _rInstances)
    {
        GFX::cVulkanMesh* pVulkanMesh = static_cast<GFX::cVulkanMesh*>(_pHandle);

        m_vulkanRenderer.DrawMeshIntances(pVulkanMesh, _rInstances);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::UpdateInstanceBuffer(std::vector<GFX::sInstanceData*>& _rInstances)
    {
        m_vulkanRenderer.UpdateInstanceBuffer(_rInstances);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::BeginDraw()
    {
        m_vulkanRenderer.BeginDraw();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::IsKeydown(int _key) const
    {
        return m_input.IsKeyDown(_key);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------