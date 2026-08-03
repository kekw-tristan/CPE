#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/instanceData.h"

#include "graphics/shapeModel/meshGenerator.h"
#include "graphics/shapeModel/shapeInstance.h"
#include "graphics/shapeModel/meshType.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>


constexpr int c_instancesPerPage    = 800;

constexpr int c_shiftKey            = 340;
constexpr int c_downArrowKey        = 264;
constexpr int c_upArrowKey          = 265;
constexpr int c_leftArrowKey        = 263;
constexpr int c_rightArrowKey       = 262;

using namespace Engine;

namespace Engine::GFX
{
    struct sTransform;
}

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
        void UpdateFreeCam(float _deltaTime);
        void UpdateThirdPersonCamera();
        void RebuildInstanceList();

        void QueueEditedModel(const Engine::GFX::sShapeModelDesc& _rModel);
        void ApplyEditedModel(const Engine::GFX::sShapeModelDesc& _rModel);
        void ClearRenderInstances();

    private:

        Engine::GFX::MeshHandle GetMesh(Engine::GFX::sMeshTypes::Enum _type);
        void BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance);
        Math::cMatrix4x4f CreateTransformMatrix(const GFX::sTransform& _rTransform);
    
    private:
    
        
    
        GFX::MeshHandle     m_cubeMesh;
        GFX::MeshHandle     m_pyramidMesh;
        GFX::sInstanceData* m_playerInstance;

        Math::cVec3f m_playerPosition;
    
        Container::cPool<GFX::sInstanceData, c_instancesPerPage> m_pool;
    
        std::vector<GFX::sInstanceData*> m_instances;
    
        std::unordered_map<GFX::MeshHandle, std::vector<GFX::sInstanceData*>> m_meshInstances;

        GFX::sShapeInstance m_playerShapeInstance;

        GFX::sShapeModelDesc m_pendingEditedModel;
        bool m_hasPendingModelUpdate = false;
    
};
