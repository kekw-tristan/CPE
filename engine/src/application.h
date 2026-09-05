#pragma once

#include "graphics/healthBarData.h"

#include <span>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace Engine
{
    namespace Logic
    {
        class cApplicationIntern;
    }

    namespace GFX
    {
        class cCamera; 
        struct sInstanceData;
    }

    struct sAppConfig
    {
        int width; 
        int height; 
        const char* pTitle;
        bool hasEditorWindows = false;
    };

    class cApplication
    {

        public:

            explicit cApplication(sAppConfig& _rAppConfig);
            virtual ~cApplication();

            cApplication(const cApplication&)               = delete; 
            cApplication& operator=(const cApplication&)    = delete; 

        public:

            void Run(); 

        protected:

            virtual void OnInit()                   {};
            virtual void OnShutdown()               {};          
            virtual void OnUpdate(float _deltaTime) {}; 
            virtual void OnPrepareRender()          {};
            virtual void OnDraw()                   {};
            virtual void OnDrawUI()                 {};

        private:

            std::unique_ptr<Logic::cApplicationIntern> m_pAppIntern; 

    };
}

namespace Engine::GFX
{
    using MeshHandle = void*;

    struct sMeshData;

    class  cModelEditorWindow;
    class  cSceneEditorWindow;

    MeshHandle CreateMesh(sMeshData& _rMeshData); 
    void SubmitMesh(MeshHandle _rMeshData);
    void DrawMeshIntances(MeshHandle _pHandle, uint32_t _instanceCount, uint32_t _firstInstances = 0);

    void UpdateInstanceBuffer(std::vector<GFX::sInstanceData*>& _rInstances);

    void UpdateHealthBars(std::span<const sHealthBarData> _healthBars);

    cCamera& GetCamera();

    cModelEditorWindow& GetModelEitorWindow();
    cSceneEditorWindow& GetSceneEditorWindow();
}

namespace Engine::Platform
{
    bool IsKeyDown(int _key); 
    bool WasMouseButtonPressed(int _button);

    float GetMouseDeltaX();
    float GetMouseDeltaY();

    void SetMouseCaptured(bool _isCaptured);

}
