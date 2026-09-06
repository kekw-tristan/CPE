#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/instanceData.h"

#include "graphics/light/lightManager.h"

#include "graphics/scene/scene.h"

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

class cEditor : public cApplication
{
    
    public:
    
        cEditor(sAppConfig& _rAppConfig);
    
    
    protected:
    
    
        void OnInit() override;
        void OnUpdate(float _deltaTime) override;
        void OnPrepareRender() override;
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

        void BuildSceneRenderInstances();
        Engine::GFX::MeshHandle GetMesh(Engine::GFX::sMeshTypes::Enum _type);
        void BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance);
        Math::cMatrix4x4f CreateTransformMatrix(const GFX::sTransform& _rTransform);
    
    private:
    

        GFX::MeshHandle m_planeMesh;
        GFX::MeshHandle m_cubeMesh;
        GFX::MeshHandle m_pyramidMesh;
        GFX::MeshHandle m_sphereMesh;
        GFX::MeshHandle m_cylinderMesh;
        GFX::MeshHandle m_coneMesh;
        GFX::MeshHandle m_torusMesh;
        GFX::MeshHandle m_crystalMesh;
        GFX::MeshHandle m_beveledCubeMesh{};
        GFX::MeshHandle m_frustumMesh{};
        GFX::MeshHandle m_wedgeMesh{};
        GFX::MeshHandle m_triangularPrismMesh{};
        GFX::MeshHandle m_icoSphereMesh{};
        GFX::MeshHandle m_rockMesh{};
        GFX::MeshHandle m_grassBladeMesh{};
        GFX::MeshHandle m_capsuleMesh{};
        GFX::MeshHandle m_archMesh{};
        GFX::MeshHandle m_extrudedPolygonMesh{};
        GFX::MeshHandle m_discMesh{};
        GFX::MeshHandle m_arcMesh{};

        GFX::sInstanceData* m_playerInstance;

        Math::cVec3f m_playerPosition;
    
        Container::cPool<GFX::sInstanceData, c_instancesPerPage> m_pool;
    
        std::vector<GFX::sInstanceData*> m_instances;
    
        std::unordered_map<GFX::MeshHandle, std::vector<GFX::sInstanceData*>> m_meshInstances;
        std::vector<GFX::LightHandle> m_modelLightHandles;

        GFX::sShapeModelDesc m_pendingEditedModel;
        bool m_hasPendingModelUpdate = false;
    
        GFX::SceneShapeInstanceHandle m_playerShapeInstanceHandle;
        GFX::SceneShapeInstanceHandle m_modelShapeInstanceHandle;

        GFX::cScene m_scene;
};
