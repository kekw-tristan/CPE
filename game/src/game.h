#pragma once

#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/instanceData.h"
#include "graphics/transform.h"

#include "graphics/scene/scene.h"

#include "graphics/shapeModel/meshType.h"
#include "graphics/shapeModel/shapeInstance.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include <unordered_map>
#include <vector>


constexpr int c_instancesPerPage = 800;

constexpr int c_downArrowKey    = 264;
constexpr int c_upArrowKey      = 265;
constexpr int c_leftArrowKey    = 263;
constexpr int c_rightArrowKey   = 262;

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

        struct sPlayerRenderPart
        {
            GFX::sInstanceData* pInstance = nullptr;
            GFX::sTransform     transform;
        };

    
    private:
    
        void InitMeshes();
        bool LoadPlayerModel();
    
        void UpdateFreeCam(float _deltaTime);

    
        void BuildSceneRenderInstances();
        void BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance);
        void BuildPlayerRenderInstances();
    
        void RebuildInstanceList();
        void ClearRenderInstances();

        void UpdatePlayer(float _deltaTime);
        void UpdatePlayerPhysics(float _deltaTime);
        void UpdatePlayerRenderInstances();
        void UpdateThirdPersonCamera(float _deltaTime); 
    
        GFX::MeshHandle GetMesh(GFX::sMeshTypes::Enum _type);
    
        Math::cMatrix4x4f CreateTransformMatrix(const GFX::sTransform& _rTransform);
    
    
    private:
    
        GFX::MeshHandle m_planeMesh;
        GFX::MeshHandle m_chunkPlaneMesh;
        GFX::MeshHandle m_cubeMesh;
        GFX::MeshHandle m_pyramidMesh;
        GFX::MeshHandle m_sphereMesh;
        GFX::MeshHandle m_cylinderMesh;
        GFX::MeshHandle m_coneMesh;
    
        Container::cPool<GFX::sInstanceData, c_instancesPerPage> m_pool;
    
        std::vector<GFX::sInstanceData*> m_instances;

        GFX::sShapeModelDesc m_playerModel;
        std::vector<sPlayerRenderPart> m_playerRenderParts;

        Math::cVec3f m_playerPosition;
        Math::cVec3f m_playerVelocity;

        bool m_isPlayerGrounded;

        float m_playerYaw;
        float m_cameraPitch;
    
        std::unordered_map<GFX::MeshHandle, std::vector<GFX::sInstanceData*>> m_meshInstances;
    
        GFX::cScene m_scene;
};