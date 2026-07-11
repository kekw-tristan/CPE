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
            OnUpdate(m_pAppIntern->GetDeltaTime());
            m_pAppIntern->Update();

            if (!m_pAppIntern->BeginFrame(m_pAppIntern->GetCamera()))
            {
                m_pAppIntern->RecreateSwapchain();
                continue;
            }
         
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
    
    void Draw(MeshHandle _pMeshHandle, std::array<float, 16>& _rWorldMatrix)
    {
        s_pApplicationIntern->Draw(_pMeshHandle, _rWorldMatrix);   
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
    bool Engine::Platform::IsKeyDown(int _key)
    {
        return s_pApplicationIntern->IsKeydown(_key);
    }
}
