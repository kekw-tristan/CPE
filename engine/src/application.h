#pragma once

#include "graphics/healthBarData.h"
#include "graphics/particles/particleData.h"
#include "graphics/gfxConfig.h"

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
        struct sBounds;
    }

    struct sAppConfig
    {
        int width; 
        int height; 
        const char* pTitle;
        bool hasEditorWindows = false;
        std::array<float, 4> backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        GFX::sEnvironmentSettings environment;
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
    // Static instances occupy the prefix; increment the revision whenever their data or order changes.
    void UpdateInstanceBuffer(std::span<const sInstanceData> _staticInstances, uint64_t _staticRevision,
        std::span<sInstanceData* const> _dynamicInstances);
    // Tests the active camera, shadow cascade/light face, or reflection-probe face.
    bool IsBoundsVisible(const sBounds& _rBounds);

    void UpdateHealthBars(std::span<const sHealthBarData> _healthBars);
    void UpdateParticles(std::span<const sParticleData> _particles);

    double GetParticleGpuMilliseconds();
    cCamera& GetCamera();

    cModelEditorWindow& GetModelEitorWindow();
    cSceneEditorWindow& GetSceneEditorWindow();
}

namespace Engine::Platform
{
    bool IsKeyDown(int _key); 
    bool IsMouseButtonDown(int _button);
    bool WasMouseButtonPressed(int _button);

    float GetMouseDeltaX();
    float GetMouseDeltaY();
    float GetMouseWheelDelta();

    void SetMouseCaptured(bool _isCaptured);

}
