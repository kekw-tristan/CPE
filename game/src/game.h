#pragma once

#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/instanceData.h"
#include "graphics/transform.h"

#include "graphics/light/lightManager.h"

#include "graphics/material/materialManager.h"

#include "graphics/scene/scene.h"

#include "graphics/shapeModel/meshType.h"
#include "graphics/shapeModel/shapeInstance.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include "physics/characterController.h"

#include "item/inventory.h"

#include "spells/runState.h"

#include "world/enemy/enemySpawn.h"

#include "enemy/enemyManager.h"
#include "enemy/projectileManager.h"

#include "ui/gameHud.h"

#include <unordered_map>
#include <array>
#include <vector>
#include <map>
#include <tuple>


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
    
        void OnInit()                   override;
        void OnUpdate(float _deltaTime) override;
        void OnPrepareRender()          override;
        void OnDraw()                   override;
        void OnShutdown()               override;
        void OnDrawUI()                 override;

    private:

        struct sPlayerRenderPart
        {
            GFX::sInstanceData* pInstance = nullptr;
            GFX::sTransform     transform;
        };

        struct sEnemyRenderPart
        {
            GFX::sInstanceData* pInstance = nullptr;
            GFX::sTransform      transform;
        };

        struct sEnemyVisual
        {
            Gameplay::sEnemyHandle handle;
            std::pair<int, int>    chunk;

            const GFX::sShapeModelDesc* pModel              = nullptr;
            uint64_t                    transformRevision   = 0;
            bool                        wasAttacking        = false;
            Math::cVec3f                previousPosition    = { 0.0f, 0.0f, 0.0f };
            float                       walkPhase           = 0.0f;
            float                       walkWeight          = 0.0f;

            std::vector<sEnemyRenderPart> renderParts;
            std::vector<GFX::LightHandle> lightHandles;
        };

        struct sWorldRenderInstances
        {
            std::vector<GFX::sInstanceData*> renderInstances;
            std::vector<GFX::LightHandle> lightHandles;
        };

        struct sProjectileVisual
        {
            uint64_t            id        = 0;
            GFX::sInstanceData* pInstance = nullptr;
            GFX::MeshHandle     mesh      = nullptr;
            GFX::LightHandle    light     = GFX::c_invalidLightHandle;
        };

    
    private:
    
        void InitMeshes();
        void InitNightSky();

        bool LoadPlayerModel();
        bool LoadEnemyModels();
        bool LoadPoseModel(const char* _pFilePath, const GFX::sShapeModelDesc& _rBaseModel, GFX::sShapeModelDesc& _rPoseModel);

        void SpawnEnemies(const std::vector<World::sEnemySpawn>& _rSpawns, const std::pair<int, int>& _rChunk);
        void RefreshWorldRenderInstances();
    
        void UpdateFreeCam(float _deltaTime);

        void BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance, sWorldRenderInstances& _rInstances);
        void BuildPlayerRenderInstances();
    
        void RebuildInstanceList();
        void ClearRenderInstances();

        void UpdatePlayer();
        void UpdatePlayerSpell(float _deltaTime);
        void UpdateInventoryInput();
        void BeginRun();
        void SyncSpellLoadoutFromInventory();
        void UpdatePlayerRenderInstances();
        void UpdateEnemyRenderInstances(float _deltaTime);

        void PrepareEnemyHealthBars(const GFX::cCamera& _rCamera);
        void SyncProjectileRenderInstances();
        void UpdateThirdPersonCamera(float _deltaTime); 
    
        GFX::MeshHandle GetMesh(GFX::sMeshTypes::Enum _type);
    
        Math::cMatrix4x4f CreateTransformMatrix(const GFX::sTransform& _rTransform);
        GFX::sTransform InterpolateTransform(const GFX::sTransform& _rFrom, const GFX::sTransform& _rTo, float _weight);
    
    private:
    
        GFX::MeshHandle m_planeMesh;
        GFX::MeshHandle m_chunkPlaneMesh;
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

        GFX::MaterialHandle m_playerSphereMaterial = -1;
    
        Container::cPool<GFX::sInstanceData, c_instancesPerPage> m_pool;
    
        std::vector<GFX::sInstanceData*> m_instances;

        GFX::sShapeModelDesc m_playerModel;
        GFX::sShapeModelDesc m_playerAttackModel;
        std::vector<sPlayerRenderPart> m_playerRenderParts;
        std::vector<GFX::LightHandle> m_playerLightHandles;

        Physics::cCharacterController m_playerController;

        float m_playerYaw;
        bool m_mouseReleased = false;
        bool m_altWasDown = false;
        float m_cameraPitch;
        float m_cameraDistance = 6.0f;
    
        std::unordered_map<GFX::MeshHandle, std::vector<GFX::sInstanceData*>> m_meshInstances;
    
        std::map<std::pair<int, int>, sWorldRenderInstances> m_worldRenderInstances;
        std::map<std::tuple<float, float, float>, Gameplay::sEnemyHandle> m_worldEnemies;
        std::vector<Gameplay::sEnemyHandle> m_bossHandles;

        bool m_enemyModelsLoaded = false;

        Gameplay::cEnemyManager        m_enemyManager;
        Gameplay::cProjectileManager   m_projectileManager;

        std::vector<sEnemyVisual>      m_enemyVisuals;

        static constexpr float c_healthBarMaxDistance   = 40.0f;
        static constexpr float c_healthBarWidth         = 1.1f;
        static constexpr float c_healthBarHeight        = 0.14f;
        static constexpr float c_crawlerHealthBarOffset = 2.8f;
        static constexpr float c_bruteHealthBarOffset   = 3.0f;

        std::vector<GFX::sHealthBarData> m_healthBars;
        std::vector<sProjectileVisual> m_projectileVisuals;

        static constexpr float c_playerMaxHealth     = 100.0f;

        UI::cGameHud m_hud;

        float m_playerHealth        = c_playerMaxHealth;
        float m_playerAttackTime    = 0.0f;
        std::array<bool, 4> m_spellKeysWasDown{};

        GFX::sShapeModelDesc m_enemy03Model;
        GFX::sShapeModelDesc m_enemy04Model;
        GFX::sShapeModelDesc m_enemy03AttackModel;
        GFX::sShapeModelDesc m_enemy04AttackModel;
        GFX::sShapeModelDesc m_thornwolfModel;
        GFX::sShapeModelDesc m_thornwolfAttackModel;
        GFX::sShapeModelDesc m_sporecapModel;
        GFX::sShapeModelDesc m_sporecapAttackModel;

        Gameplay::cInventory m_inventory;
        Gameplay::cRunState m_runState;

        bool m_inventoryOpen        = false;
        bool m_inventoryKeyWasDown  = false;
        bool m_escapeKeyWasDown     = false;
};
