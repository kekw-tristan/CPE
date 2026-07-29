#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/meshGenerator.h"
#include "graphics/instanceData.h"

#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>


constexpr int c_instancesPerPage = 800;

constexpr int c_shiftKey = 340;
constexpr int c_downArrowKey = 264;
constexpr int c_upArrowKey = 265;
constexpr int c_leftArrowKey = 263;
constexpr int c_rightArrowKey = 262;

using namespace Engine;

class cGame : public cApplication
{
    
    public:
    
        cGame(sAppConfig& _rAppConfig);
    
    
    protected:
    
    
        void OnInit() override;
        void OnUpdate(float _deltaTime) override;
        void OnDraw() override;
        void OnShutdown() override;
    
    private:
    
        void UpdatePlayer(float _deltaTime);
        void UpdateThirdPersonCamera();
        void RebuildInstanceList();
    
    private:
    
        
    
        GFX::MeshHandle     m_cubeMesh;
        GFX::MeshHandle     m_pyramidMesh;
        GFX::sInstanceData* m_playerInstance;

        Math::cVec3f m_playerPosition;
    
        Container::cPool<GFX::sInstanceData, c_instancesPerPage> m_pool;
    
        std::vector<GFX::sInstanceData*> m_instances;
    
        std::unordered_map<GFX::MeshHandle, std::vector<GFX::sInstanceData*>> m_meshInstances;
    
};
