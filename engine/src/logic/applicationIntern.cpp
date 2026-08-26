#include "applicationIntern.h"

#include "application.h"

#include "graphics/vertex.h"
#include "graphics/instanceData.h"
#include "graphics/meshData.h"

#include "graphics/vulkan/vulkanBuffer.h"
#include "graphics/vulkan/vulkanVertex.h"

#include "graphics/imgui/imguiManager.h"
#include "graphics/imgui/imguiWindowManager.h"

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

        InitializeImGui();

        m_camera.LookAt(
            2.0f, 1.5f, 3.0f,
            0.0f, 0.0f, 0.0f
        );

        m_camera.SetPerspective(
            60.0f,
            0.1f,
            10000.0f
        );

        m_Timer.Reset();
        m_input.Init(m_window.GetWindow());
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // application destructor

    cApplicationIntern::~cApplicationIntern()
    {
        m_vulkanDevice.WaitIdle();

        GFX::ImGuiManager::Shutdown(); 

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
        m_frameStatsWindow.GetFrameWindowStats().drawCalls = 0;
        m_frameStatsWindow.GetFrameWindowStats().instances = 0;

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

        m_frameStatsWindow.GetFrameWindowStats().deltaTime = deltaTime;

        m_modelEditorWindow.Update(m_input, m_camera);
        m_sceneEditorWindow.Update(m_input, m_camera);
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
        m_vulkanRenderer.RecreateColorBuffer();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GFX::cCamera& cApplicationIntern::GetCamera()
    {
        return m_camera;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::WasResized()
    {
        return m_window.WasResized();
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

    void cApplicationIntern::DrawMeshIntances(GFX::MeshHandle _pHandle, uint32_t _instanceCount, uint32_t _firstInstance)
    {
        auto& stats = m_frameStatsWindow.GetFrameWindowStats();

        ++stats.drawCalls;
        stats.instances += _instanceCount;

        GFX::cVulkanMesh* pVulkanMesh = static_cast<GFX::cVulkanMesh*>(_pHandle);

        m_vulkanRenderer.DrawMeshIntances(pVulkanMesh, _instanceCount, _firstInstance);
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

    void cApplicationIntern::BeginShadowRendering()
    {
        m_vulkanRenderer.BeginShadowRendering();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::EndShadowRendering()
    {
        m_vulkanRenderer.EndShadowRendering();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::BeginShadowDraw(uint32_t _shadowIndex, uint32_t _matrixIndex)
    {
        m_vulkanRenderer.BeginShadowDraw(_shadowIndex, _matrixIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::EndShadowDraw()
    {
        m_vulkanRenderer.EndShadowDraw();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::NeedsReflectionProbeUpdate(uint32_t _probeIndex) const
    {
        return m_vulkanRenderer.NeedsReflectionProbeUpdate(_probeIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::BeginReflectionProbeRendering(uint32_t _probeIndex)
    {
        m_vulkanRenderer.BeginReflectionProbeRendering(_probeIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::BeginReflectionProbeDraw(uint32_t _faceIndex)
    {
        m_vulkanRenderer.BeginReflectionProbeDraw(_faceIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::EndReflectionProbeDraw()
    {
        m_vulkanRenderer.EndReflectionProbeDraw();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::EndReflectionProbeRendering()
    {
        m_vulkanRenderer.EndReflectionProbeRendering();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::PrefilterReflectionProbe(uint32_t _probeIndex)
    {
        m_vulkanRenderer.PrefilterReflectionProbe(_probeIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cApplicationIntern::GetReflectionProbeCount() const
    {
        return m_vulkanRenderer.GetReflectionProbeCount();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cApplicationIntern::GetShadowCount() const
    {
        return m_vulkanRenderer.GetShadowCount();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cApplicationIntern::GetShadowMatrixCount(uint32_t _shadowIndex) const
    {
        return m_vulkanRenderer.GetShadowMatrixCount(_shadowIndex);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cApplicationIntern::GetMouseDeltaX()
    {
        return static_cast<float>(m_input.GetMouseDeltaX());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cApplicationIntern::GetMouseDeltaY()
    {
        return static_cast<float>(m_input.GetMouseDeltaY());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GFX::cModelEditorWindow& cApplicationIntern::GetModelEitorWindow()
    {
        return m_modelEditorWindow;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GFX::cSceneEditorWindow& cApplicationIntern::GetSceneEditorWindow()
    {
        return m_sceneEditorWindow;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cApplicationIntern::IsKeydown(int _key) const
    {
        return m_input.IsKeyDown(_key);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplicationIntern::InitializeImGui()
    {
        // init desc

        GFX::sImGuiInitDesc imGuiInitDesc{};

        imGuiInitDesc.pWindow = m_window.GetWindow();

        imGuiInitDesc.instance              = m_vulkanContext.GetInstance();
        imGuiInitDesc.physicalDevice        = m_vulkanDevice.GetPhysicalDevice();
        imGuiInitDesc.device                = m_vulkanDevice.GetDevice();

        imGuiInitDesc.graphicsQueue         = m_vulkanDevice.GetGraphicsQueue();
        imGuiInitDesc.graphicsQueueFamily   = m_vulkanDevice.GetQueueFamilyIndices().graphicsFamily;

        imGuiInitDesc.renderPass            = VK_NULL_HANDLE;
        imGuiInitDesc.descriptorPool        = m_vulkanRenderer.GetImguiDescriptorPool();

        imGuiInitDesc.imageCount            = m_vulkanSwapchain.GetImageCount();
        imGuiInitDesc.minImageCount         = m_vulkanSwapchain.GetImageCount();

        imGuiInitDesc.colorFormat           = m_vulkanSwapchain.GetImageFormat();
        imGuiInitDesc.depthFormat           = VK_FORMAT_D32_SFLOAT;

        imGuiInitDesc.msaaSamples           = m_vulkanDevice.GetMSAASamples();

        GFX::ImGuiManager::Init(imGuiInitDesc);

        // add windows

        GFX::ImGuiWindowManager::AddWindow(m_frameStatsWindow);
        GFX::ImGuiWindowManager::AddWindow(m_modelEditorWindow);
        GFX::ImGuiWindowManager::AddWindow(m_sceneEditorWindow);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------