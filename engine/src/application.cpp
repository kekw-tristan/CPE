#include "application.h"

#include "graphics/imgui/imguiWindowManager.h"
#include "graphics/imgui/modelEditorWindow.h"

#include "logic/applicationIntern.h"

#include <imgui.h>
#include <iostream>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine
{

    static Logic::cApplicationIntern* s_pApplicationIntern = nullptr;

    // -------------------------------------------------------------------------------------------------------------------------

    cApplication::cApplication(sAppConfig& _rAppConfig)
        : m_pAppIntern(std::make_unique<Logic::cApplicationIntern>(_rAppConfig))
    {
        s_pApplicationIntern = m_pAppIntern.get(); 
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cApplication::~cApplication()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------
    
    void cApplication::Run()
    {
        OnInit();

        while(!m_pAppIntern->GetShouldClose())
        {
            if (m_pAppIntern->WasResized())
            {
                m_pAppIntern->RecreateSwapchain();
                continue;
            }
            m_pAppIntern->Update();     
            

            if (!m_pAppIntern->BeginFrame(m_pAppIntern->GetCamera()))
            {
                m_pAppIntern->RecreateSwapchain();
                continue;
            }
            
            OnUpdate(m_pAppIntern->GetDeltaTime());     

            m_pAppIntern->BeginShadowRendering();

            for (uint32_t shadowIndex = 0; shadowIndex < m_pAppIntern->GetShadowCount(); ++shadowIndex)
            {
                const uint32_t matrixCount = m_pAppIntern->GetShadowMatrixCount(shadowIndex);

                for (uint32_t matrixIndex = 0; matrixIndex < matrixCount; ++matrixIndex)
                {
                    m_pAppIntern->BeginShadowDraw(shadowIndex, matrixIndex);

                    OnDraw();

                    m_pAppIntern->EndShadowDraw();
                }
            }

            m_pAppIntern->EndShadowRendering();


            m_pAppIntern->BeginDraw();
            OnDraw();
            
            GFX::ImGuiWindowManager::Draw(); 

            if (!m_pAppIntern->EndFrame())
            {
                m_pAppIntern->RecreateSwapchain();
                continue;
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{
    // -------------------------------------------------------------------------------------------------------------------------

    MeshHandle CreateMesh(sMeshData& _rMeshData)
    {
        return s_pApplicationIntern->CreateMesh(_rMeshData);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void SubmitMesh(MeshHandle _pMeshHandle)
    {
        if (s_pApplicationIntern == nullptr)
        {
            throw std::runtime_error("Application Inter does not exist yet!");
        }

        if (_pMeshHandle == nullptr)
        {
            throw std::runtime_error("Submitted Mesh nullptr!");
        }

        s_pApplicationIntern->SubmitMesh(_pMeshHandle);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void DrawMeshIntances(MeshHandle _pHandle, uint32_t _instanceCount, uint32_t _firstInstance)
    {
        s_pApplicationIntern->DrawMeshIntances(_pHandle, _instanceCount, _firstInstance);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void UpdateInstanceBuffer(std::vector<GFX::sInstanceData*>& _rInstances)
    {
        s_pApplicationIntern->UpdateInstanceBuffer(_rInstances);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cCamera& GetCamera()
    {
        return s_pApplicationIntern->GetCamera();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cModelEditorWindow& GetModelEitorWindow()
    {
        return s_pApplicationIntern->GetModelEitorWindow();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cSceneEditorWindow& GetSceneEditorWindow()
    {
        return s_pApplicationIntern->GetSceneEditorWindow();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------


namespace Engine::Platform
{
    bool IsKeyDown(int _key)
    {
        return s_pApplicationIntern->IsKeydown(_key);
    }
}
