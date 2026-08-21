#pragma once

#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/instanceData.h"

#include "graphics/scene/scene.h"

#include "graphics/shapeModel/meshType.h"
#include "graphics/shapeModel/shapeInstance.h"

#include <unordered_map>
#include <vector>


constexpr int c_instancesPerPage = 800;

constexpr int c_downArrowKey    = 264;
constexpr int c_upArrowKey      = 265;
constexpr int c_leftArrowKey    = 263;
constexpr int c_rightArrowKey   = 262;

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
    
        void InitMeshes();
    
        void UpdateFreeCam(float _deltaTime);
    
        void BuildSceneRenderInstances();
        void BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance);
    
        void RebuildInstanceList();
        void ClearRenderInstances();
    
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
    
        std::unordered_map<GFX::MeshHandle, std::vector<GFX::sInstanceData*>> m_meshInstances;
    
        GFX::cScene m_scene;
};