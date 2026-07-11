#pragma once

#include <array>
#include <memory>

namespace Engine
{
    namespace Logic
    {
        class cApplicationIntern;
    }

    namespace GFX
    {
        class cCamera; 
    }

    struct sAppConfig
    {
        int width; 
        int height; 
        const char* _pTitle;
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
            virtual void OnDraw()                   {};            

        private:

            std::unique_ptr<Logic::cApplicationIntern> m_pAppIntern; 

    };
}

namespace Engine::GFX
{
    using MeshHandle = void*;

    struct sMeshData;

    MeshHandle CreateMesh(sMeshData& _rMeshData); 
    void SubmitMesh(MeshHandle _rMeshData);
    void Draw(MeshHandle _rMeshData, std::array<float, 16>& _rWorldMatrix);

    cCamera& GetCamera();
}

namespace Engine::Platform
{
    bool IsKeyDown(int _key); 
}