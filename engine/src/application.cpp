#include "application.h"

#include "logic/applicationIntern.h"

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
            m_pAppIntern->Update();

            if (!m_pAppIntern->BeginFrame(m_pAppIntern->GetCamera()))
            {
                m_pAppIntern->RecreateSwapchain();
                continue;
            }
           
            OnUpdate(m_pAppIntern->GetDeltaTime());

            m_pAppIntern->BeginDraw();
            OnDraw();
            
            
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

    void DrawMeshIntances(MeshHandle _pHandle, std::vector<sInstanceData*>& _rInstances)
    {
        s_pApplicationIntern->DrawMeshIntances(_pHandle, _rInstances);
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

}

// -------------------------------------------------------------------------------------------------------------------------


namespace Engine::Platform
{
    bool IsKeyDown(int _key)
    {
        return s_pApplicationIntern->IsKeydown(_key);
    }
}
