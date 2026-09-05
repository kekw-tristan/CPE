#include "game.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "world/worldGenerator.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

// -------------------------------------------------------------------------------------------------------------------------

cGame::cGame(Engine::sAppConfig& _rAppConfig)
    : cApplication(_rAppConfig)
    , m_planeMesh()
    , m_chunkPlaneMesh()
    , m_cubeMesh()
    , m_pyramidMesh()
    , m_sphereMesh()
    , m_cylinderMesh()
    , m_coneMesh()
    , m_torusMesh()
    , m_crystalMesh()
    , m_pool()
    , m_instances()
    , m_playerModel()
    , m_playerRenderParts()
    , m_playerController()
    , m_playerYaw(0.f)
    , m_cameraPitch(-20.f)
    , m_meshInstances()
    , m_scene()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    InitMeshes();

    World::WorldGenerator::Generate(m_scene, 1337);

    BuildSceneRenderInstances();

    if (LoadPlayerModel())
        BuildPlayerRenderInstances();

    if (LoadEnemyModels())
        SpawnEnemies();

    UpdateEnemyRenderInstances();

    RebuildInstanceList();

    Platform::SetMouseCaptured(true);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
    //UpdateFreeCam(_deltaTime);

    UpdatePlayer();

    m_playerController.Update(_deltaTime);
    UpdateThirdPersonCamera(_deltaTime);
    UpdatePlayerSpell(_deltaTime);

    Gameplay::sEnemyUpdateContext enemyContext{};
    enemyContext.deltaTime      = _deltaTime;
    enemyContext.playerPosition = m_playerController.GetPosition();

    m_enemyManager.Update(enemyContext, m_projectileManager);
    m_projectileManager.Update(_deltaTime, enemyContext.playerPosition, m_enemyManager);

    const float receivedDamage = m_enemyManager.ConsumePlayerDamage() + m_projectileManager.ConsumePlayerDamage();
    if (receivedDamage > 0.0f)
    {
        m_playerHealth = std::max(0.0f, m_playerHealth - receivedDamage);
        std::cout << "Player health: " << m_playerHealth << '\n';
    }

    UpdatePlayerRenderInstances();
    UpdateEnemyRenderInstances();
    SyncProjectileRenderInstances();

}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnPrepareRender()
{
    Engine::GFX::UpdateInstanceBuffer(m_instances);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnDraw()
{
    uint32_t firstInstance = 0;

    for (auto& [mesh, instances] : m_meshInstances)
    {
        Engine::GFX::DrawMeshIntances(mesh, static_cast<uint32_t>(instances.size()), firstInstance);

        firstInstance += static_cast<uint32_t>(instances.size());
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnDrawUI()
{
    UI::sHudState hudState;

    hudState.health                = m_playerHealth;
    hudState.maxHealth             = c_playerMaxHealth;
    hudState.spellCooldown         = m_playerSpellCooldown;
    hudState.spellCooldownDuration = c_playerSpellCooldown;

    m_hud.Draw(hudState);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnShutdown()
{
    m_enemyManager.Clear();
    m_projectileManager.Clear();
    m_enemyVisuals.clear();
    m_projectileVisuals.clear();
    ClearRenderInstances();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::InitMeshes()
{
    using namespace Engine::GFX;

    sMeshData& planeData    = ShapeMeshLibrary::GetMeshData(sMeshTypes::Plane);
    sMeshData& chunkPlane   = ShapeMeshLibrary::GetMeshData(sMeshTypes::ChunkPlane);
    sMeshData& cubeData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    sMeshData& pyramidData  = ShapeMeshLibrary::GetMeshData(sMeshTypes::Pyramid);
    sMeshData& sphereData   = ShapeMeshLibrary::GetMeshData(sMeshTypes::Sphere);
    sMeshData& cylinderData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cylinder);
    sMeshData& coneData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cone);

    m_planeMesh      = CreateMesh(planeData);
    m_chunkPlaneMesh = CreateMesh(chunkPlane);
    m_cubeMesh       = CreateMesh(cubeData);
    m_pyramidMesh    = CreateMesh(pyramidData);
    m_sphereMesh     = CreateMesh(sphereData);
    m_cylinderMesh   = CreateMesh(cylinderData);
    m_coneMesh       = CreateMesh(coneData);

    SubmitMesh(m_planeMesh);
    SubmitMesh(m_chunkPlaneMesh);
    SubmitMesh(m_cubeMesh);
    SubmitMesh(m_pyramidMesh);
    SubmitMesh(m_sphereMesh);
    SubmitMesh(m_cylinderMesh);
    SubmitMesh(m_coneMesh);

    m_torusMesh = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Torus));
    m_crystalMesh = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Crystal));
    SubmitMesh(m_torusMesh);
    SubmitMesh(m_crystalMesh);

    sLight directionalLight0{};
    
    directionalLight0.type          = sLightType::Directional;
    directionalLight0.color         = { 0.8f, 0.8f, 0.8f };
    directionalLight0.intensity     = 2.5f;
    directionalLight0.direction     = { -0.5f, -0.5f, -0.3f };
    directionalLight0.castsShadow   = true;
    
    LightManager::CreateLight(directionalLight0);
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadPlayerModel()
{
    std::string errorMessage;

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/player_wizard.json", m_playerModel, errorMessage))
    {
        std::cerr << "Failed to load player model: " << errorMessage << "\n"; 
        return false;
    }

    LoadPoseModel("./assets/models/player_wizard_attack.json", m_playerModel, m_playerAttackModel);
    return true;
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadEnemyModels()
{
    std::string errorMessage;

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/enemy_03.json", m_enemy03Model, errorMessage))
    {
        std::cerr << "Failed to load enemy_03: " << errorMessage << '\n';
        return false;
    }

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/enemy_04.json", m_enemy04Model, errorMessage))
    {
        std::cerr << "Failed to load enemy_04: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/enemy_03_attack.json", m_enemy03Model, m_enemy03AttackModel);
    LoadPoseModel("./assets/models/enemy_04_attack.json", m_enemy04Model, m_enemy04AttackModel);
    return true;
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadPoseModel(const char* _pFilePath, const GFX::sShapeModelDesc& _rBaseModel, GFX::sShapeModelDesc& _rPoseModel)
{
    std::string errorMessage;
    if (!GFX::ShapeModelLoader::LoadFromFile(_pFilePath, _rPoseModel, errorMessage))
        return false;

    if (_rPoseModel.shapes.size() != _rBaseModel.shapes.size())
    {
        std::cerr << "Pose model has a different shape count: " << _pFilePath << '\n';
        _rPoseModel = {};
        return false;
    }

    for (size_t shapeIndex = 0; shapeIndex < _rBaseModel.shapes.size(); ++shapeIndex)
    {
        if (_rPoseModel.shapes[shapeIndex].meshType != _rBaseModel.shapes[shapeIndex].meshType)
        {
            std::cerr << "Pose model has a different mesh type at shape " << shapeIndex << ": " << _pFilePath << '\n';
            _rPoseModel = {};
            return false;
        }
    }

    return true;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::SpawnEnemies()
{
    const std::vector<World::sEnemySpawn>& spawns = World::WorldGenerator::GetEnemySpawns();

    m_enemyVisuals.reserve(spawns.size());

    for (const World::sEnemySpawn& spawn : spawns)
    {
        const GFX::sShapeModelDesc* pModel = nullptr;

        switch (spawn.type)
        {
        case World::sEnemyType::ForestCrawler:
            pModel = &m_enemy03Model;
            break;

        case World::sEnemyType::ForestBrute:
            pModel = &m_enemy04Model;
            break;
        }

        if (pModel == nullptr)
            continue;

        sEnemyVisual visual{};
        visual.handle = m_enemyManager.Spawn(spawn.type, spawn.position, spawn.rotation);
        visual.renderParts.reserve(pModel->shapes.size());

        for (const GFX::sShapePartDesc& part : pModel->shapes)
        {
            GFX::sInstanceData* pInstance = m_pool.Create();

            pInstance->color =
            {
                part.color[0],
                part.color[1],
                part.color[2],
                part.color[3]
            };

            pInstance->materialIndex = part.materialIndex;

            GFX::MeshHandle mesh = GetMesh(part.meshType);

            m_meshInstances[mesh].push_back(pInstance);

            sEnemyRenderPart renderPart{};

            renderPart.pInstance = pInstance;
            renderPart.transform = part.transform;

            visual.renderParts.push_back(renderPart);
        }

        m_enemyVisuals.push_back(std::move(visual));
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateFreeCam(float _deltaTime)
{
    using namespace Engine::Platform;

    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();

    constexpr float moveSpeed = 10.0f;
    constexpr float rotationSpeed = 100.0f;

    if (IsKeyDown(c_downArrowKey))
        rCamera.AddPitch(-rotationSpeed * _deltaTime);

    if (IsKeyDown(c_upArrowKey))
        rCamera.AddPitch(rotationSpeed * _deltaTime);

    if (IsKeyDown(c_leftArrowKey))
        rCamera.AddYaw(-rotationSpeed * _deltaTime);

    if (IsKeyDown(c_rightArrowKey))
        rCamera.AddYaw(rotationSpeed * _deltaTime);

    float direction[4];
    rCamera.GetDirection(direction);

    Engine::Math::cVec3f forward(direction[0], direction[1], direction[2]);
    forward.normalize();

    Engine::Math::cVec3f right(-forward.z(), 0.0f, forward.x());
    right.normalize();

    Engine::Math::cVec3f movement;

    if (IsKeyDown('W'))
        movement += forward * moveSpeed * _deltaTime;

    if (IsKeyDown('S'))
        movement -= forward * moveSpeed * _deltaTime;

    if (IsKeyDown('A'))
        movement -= right * moveSpeed * _deltaTime;

    if (IsKeyDown('D'))
        movement += right * moveSpeed * _deltaTime;

    if (IsKeyDown('Q'))
        movement += Engine::Math::cVec3f(0.0f, -moveSpeed * _deltaTime, 0.0f);

    if (IsKeyDown('E'))
        movement += Engine::Math::cVec3f(0.0f, moveSpeed * _deltaTime, 0.0f);

    if (movement.isZero())
        return;

    float position[4];
    rCamera.GetPosition(position);

    Engine::Math::cVec3f cameraPosition(position[0], position[1], position[2]);

    cameraPosition += movement;

    rCamera.SetPosition(cameraPosition.x(), cameraPosition.y(), cameraPosition.z());
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BuildSceneRenderInstances()
{
    for (const Engine::GFX::sShapeInstance& shapeInstance : m_scene.GetShapeInstances())
        BuildRenderInstances(shapeInstance);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance)
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    const sShapeModelDesc& model = ShapeModelManager::GetShapeModel(_rShapeInstance.modelHandle);

    cMatrix4x4f instanceMatrix = CreateTransformMatrix(_rShapeInstance.transform);

    for (const sShapePartDesc& part : model.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        cMatrix4x4f partMatrix = CreateTransformMatrix(part.transform);

        pInstance->worldMatrix = partMatrix * instanceMatrix;

        pInstance->color =
        {
            part.color[0],
            part.color[1],
            part.color[2],
            part.color[3]
        };

        pInstance->materialIndex = part.materialIndex;

        MeshHandle mesh = GetMesh(part.meshType);

        if (part.meshType == sMeshTypes::ChunkPlane)
        {
            pInstance->instanceFlags |= sInstanceFlags::InstanceFlagTerrain;
        }

        m_meshInstances[mesh].push_back(pInstance);
    }
}

// -------------------------------------------------------------------------------------------------------------------------


void cGame::BuildPlayerRenderInstances()
{
    using namespace Engine::GFX;

    m_playerRenderParts.clear();
    m_playerRenderParts.reserve(m_playerModel.shapes.size());

    for (const sShapePartDesc& rPart : m_playerModel.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        pInstance->color =
        {
            rPart.color[0],
            rPart.color[1],
            rPart.color[2],
            rPart.color[3]
        };

        pInstance->materialIndex = rPart.materialIndex;

        MeshHandle mesh = GetMesh(rPart.meshType);

        m_meshInstances[mesh].push_back(pInstance);

        sPlayerRenderPart renderPart{};

        renderPart.pInstance = pInstance;
        renderPart.transform = rPart.transform;

        m_playerRenderParts.push_back(renderPart);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::RebuildInstanceList()
{
    m_instances.clear();

    for (auto& [mesh, instances] : m_meshInstances)
    {
        for (Engine::GFX::sInstanceData* pInstance : instances)
            m_instances.push_back(pInstance);
    }

}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ClearRenderInstances()
{
    for (Engine::GFX::sInstanceData* pInstance : m_instances)
        m_pool.Destroy(pInstance);

    m_instances.clear();
    m_meshInstances.clear();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayer()
{
    using namespace Engine;
    using namespace Engine::Platform;

    constexpr float c_moveSpeed = 6.0f;
    constexpr float c_jumpVelocity = 6.0f;

    GFX::cCamera& rCamera = GFX::GetCamera();

    float direction[4];
    rCamera.GetDirection(direction);

    Math::cVec3f forward(direction[0], 0.0f, direction[2]);
    Math::cVec3f movement;

    if (!forward.isZero())
    {
        forward.normalize();

        Math::cVec3f right(-forward.z(), 0.0f, forward.x());
        right.normalize();

        if (IsKeyDown('W'))
            movement += forward;

        if (IsKeyDown('S'))
            movement -= forward;

        if (IsKeyDown('A'))
            movement -= right;

        if (IsKeyDown('D'))
            movement += right;
    }

    if (!movement.isZero())
    {
        movement.normalize();

        m_playerYaw = std::atan2(movement.x(), movement.z());
    }

    m_playerController.Move(movement, c_moveSpeed);

    if (IsKeyDown(32))
        m_playerController.Jump(c_jumpVelocity);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayerSpell(float _deltaTime)
{
    constexpr float c_spellSpeed     = 13.0f;
    constexpr float c_spellDamage    = 25.0f;
    constexpr int   c_leftMouseButton = 0;

    m_playerSpellCooldown = std::max(0.0f, m_playerSpellCooldown - _deltaTime);
    m_playerAttackTime = std::max(0.0f, m_playerAttackTime - _deltaTime);

    if (!Engine::Platform::WasMouseButtonPressed(c_leftMouseButton) || m_playerSpellCooldown > 0.0f)
        return;

    float cameraDirection[4];
    Engine::GFX::GetCamera().GetDirection(cameraDirection);

    Engine::Math::cVec3f direction(cameraDirection[0], 0.0f, cameraDirection[2]);
    direction.normalize();

    if (direction.isZero())
        return;

    Gameplay::sProjectileSpawnDesc projectile{};
    projectile.position  = m_playerController.GetPosition() + Engine::Math::cVec3f(0.0f, 1.25f, 0.0f) + direction * 0.8f;
    projectile.direction = direction;
    projectile.speed     = c_spellSpeed;
    projectile.damage    = c_spellDamage;
    projectile.lifetime  = 2.5f;

    m_projectileManager.SpawnPlayerSphere(projectile);

    m_playerSpellCooldown = c_playerSpellCooldown;
    m_playerAttackTime = 0.4f;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayerRenderInstances()
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    if (m_playerRenderParts.empty())
        return;

    sTransform playerTransform{};

    playerTransform.position = m_playerController.GetPosition();
    playerTransform.rotation = { 0.0f, m_playerYaw, 0.0f };
    playerTransform.scale = { 1.0f, 1.0f, 1.0f };

    const cMatrix4x4f playerMatrix = CreateTransformMatrix(playerTransform);

    float attackWeight = 1.0f - std::abs(m_playerAttackTime - 0.2f) / 0.2f;
    attackWeight = std::clamp(attackWeight, 0.0f, 1.0f);
    attackWeight = attackWeight * attackWeight * (3.0f - 2.0f * attackWeight);

    for (size_t partIndex = 0; partIndex < m_playerRenderParts.size(); ++partIndex)
    {
        sPlayerRenderPart& renderPart = m_playerRenderParts[partIndex];
        const sTransform partTransform = m_playerAttackModel.shapes.empty() ? renderPart.transform : InterpolateTransform(renderPart.transform, m_playerAttackModel.shapes[partIndex].transform, attackWeight);
        const cMatrix4x4f partMatrix = CreateTransformMatrix(partTransform);

        renderPart.pInstance->worldMatrix = partMatrix * playerMatrix;
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateEnemyRenderInstances()
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    for (sEnemyVisual& visual : m_enemyVisuals)
    {
        const Gameplay::sEnemy* pEnemy = m_enemyManager.TryGetEnemy(visual.handle);

        if (pEnemy == nullptr)
            continue;

        const bool isAttacking = pEnemy->state == Gameplay::eEnemyState::AttackWindup || pEnemy->state == Gameplay::eEnemyState::AttackRecovery;

        if (visual.transformRevision == pEnemy->transformRevision && !isAttacking && !visual.wasAttacking)
            continue;

        sTransform enemyTransform{};

        enemyTransform.position = pEnemy->position;
        enemyTransform.rotation = { 0.0f, pEnemy->rotation, 0.0f };
        enemyTransform.scale = pEnemy->state == Gameplay::eEnemyState::Dead
            ? Math::cVec3f(0.0f, 0.0f, 0.0f)
            : Math::cVec3f(1.0f, 1.0f, 1.0f);

        const cMatrix4x4f enemyMatrix = CreateTransformMatrix(enemyTransform);

        const sShapeModelDesc& attackModel = pEnemy->type == World::sEnemyType::ForestCrawler ? m_enemy03AttackModel : m_enemy04AttackModel;

        for (size_t partIndex = 0; partIndex < visual.renderParts.size(); ++partIndex)
        {
            sEnemyRenderPart& renderPart    = visual.renderParts[partIndex];
            const sTransform partTransform  = attackModel.shapes.empty() ? renderPart.transform : InterpolateTransform(renderPart.transform, attackModel.shapes[partIndex].transform, pEnemy->attackPoseWeight);
            const cMatrix4x4f partMatrix    = CreateTransformMatrix(partTransform);

            renderPart.pInstance->worldMatrix = partMatrix * enemyMatrix;
        }

        visual.transformRevision = pEnemy->transformRevision;
        visual.wasAttacking = isAttacking;
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::SyncProjectileRenderInstances()
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    const std::vector<Gameplay::sProjectile>& projectiles = m_projectileManager.GetProjectiles();
    std::unordered_set<uint64_t> activeIds;
    activeIds.reserve(projectiles.size());

    bool instanceListChanged = false;

    for (const Gameplay::sProjectile& projectile : projectiles)
    {
        activeIds.insert(projectile.id);

        auto visual = std::find_if(m_projectileVisuals.begin(), m_projectileVisuals.end(), [&projectile](const sProjectileVisual& _rVisual)
        {
            return _rVisual.id == projectile.id;
        });

        if (visual == m_projectileVisuals.end())
        {
            sInstanceData* pInstance = m_pool.Create();
            const bool isPlayerSpell = projectile.type == Gameplay::eProjectileType::PlayerSphere;

            pInstance->color = isPlayerSpell
                ? std::array<float, 4>{ 0.5f, 0.15f, 1.0f, 1.0f }
                : std::array<float, 4>{ 0.08f, 0.9f, 1.0f, 1.0f };

            pInstance->materialIndex = m_playerModel.materialIndices.size() > 3 ? m_playerModel.materialIndices[3] : 0;

            const MeshHandle mesh = isPlayerSpell ? m_sphereMesh : m_coneMesh;

            m_meshInstances[mesh].push_back(pInstance);
            m_projectileVisuals.push_back({ projectile.id, pInstance, mesh });

            visual = std::prev(m_projectileVisuals.end());
            instanceListChanged = true;
        }

        sTransform transform{};
        transform.position = projectile.position;

        if (projectile.type == Gameplay::eProjectileType::PlayerSphere)
        {
            transform.rotation = { 0.0f, 0.0f, 0.0f };
            transform.scale = { 0.42f, 0.42f, 0.42f };
        }
        else
        {
            transform.rotation = { 1.5707963f, std::atan2(projectile.direction.x(), projectile.direction.z()), 0.0f };
            transform.scale = { 0.18f, 0.42f, 0.18f };
        }

        visual->pInstance->worldMatrix = CreateTransformMatrix(transform);
    }

    auto visual = m_projectileVisuals.begin();
    while (visual != m_projectileVisuals.end())
    {
        if (activeIds.contains(visual->id))
        {
            ++visual;
            continue;
        }

        std::vector<sInstanceData*>& meshInstances = m_meshInstances[visual->mesh];
        std::erase(meshInstances, visual->pInstance);
        m_pool.Destroy(visual->pInstance);
        visual = m_projectileVisuals.erase(visual);
        instanceListChanged = true;
    }

    if (instanceListChanged)
        RebuildInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateThirdPersonCamera(float _deltaTime)
{
    using namespace Engine;

    constexpr float c_mouseSensitivity  = 0.12f;
    constexpr float c_cameraDistance    = 7.5f;
    constexpr float c_targetHeight      = 1.7f;
    constexpr float c_minPitch          = -60.0f;
    constexpr float c_maxPitch          = -10.0f;

    GFX::cCamera& rCamera = GFX::GetCamera();

    const float mouseDeltaX = static_cast<float>(Platform::GetMouseDeltaX());
    const float mouseDeltaY = static_cast<float>(Platform::GetMouseDeltaY());

    rCamera.AddYaw(mouseDeltaX * c_mouseSensitivity);

    const float pitchChange = -mouseDeltaY * c_mouseSensitivity;
    const float newPitch    = std::clamp(m_cameraPitch + pitchChange, c_minPitch, c_maxPitch);

    rCamera.AddPitch(newPitch - m_cameraPitch);
    m_cameraPitch = newPitch;

    float direction[4];
    rCamera.GetDirection(direction);

    Math::cVec3f cameraDirection(direction[0], direction[1], direction[2]);
    cameraDirection.normalize();

    const Math::cVec3f targetPosition = m_playerController.GetPosition() + Math::cVec3f(0.0f, c_targetHeight, 0.0f);
    const Math::cVec3f cameraPosition = targetPosition - cameraDirection * c_cameraDistance;

    rCamera.SetPosition(cameraPosition.x(), cameraPosition.y(), cameraPosition.z());
}

// -------------------------------------------------------------------------------------------------------------------------

Engine::GFX::MeshHandle cGame::GetMesh(Engine::GFX::sMeshTypes::Enum _type)
{
    using namespace Engine::GFX;

    switch (_type)
    {
        case sMeshTypes::Plane:
            return m_planeMesh;

        case sMeshTypes::ChunkPlane:
            return m_chunkPlaneMesh;

        case sMeshTypes::Cube:
            return m_cubeMesh;

        case sMeshTypes::Pyramid:
            return m_pyramidMesh;

        case sMeshTypes::Sphere:
            return m_sphereMesh;

        case sMeshTypes::Cylinder:
            return m_cylinderMesh;

        case sMeshTypes::Cone:
            return m_coneMesh;

        case sMeshTypes::Torus:
            return m_torusMesh;

        case sMeshTypes::Crystal:
            return m_crystalMesh;

        default:
            return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------------

Math::cMatrix4x4f cGame::CreateTransformMatrix(const GFX::sTransform& _rTransform)
{
    using namespace Engine::Math;

    cMatrix4x4f translation = cMatrix4x4f::translation(_rTransform.position);
    cMatrix4x4f scale       = cMatrix4x4f::scale(_rTransform.scale);
    cMatrix4x4f rotation    = cMatrix4x4f::rotationX(_rTransform.rotation.x()) * cMatrix4x4f::rotationY(_rTransform.rotation.y()) * cMatrix4x4f::rotationZ(_rTransform.rotation.z());

    return scale * rotation * translation;
}

// -------------------------------------------------------------------------------------------------------------------------

GFX::sTransform cGame::InterpolateTransform(const GFX::sTransform& _rFrom, const GFX::sTransform& _rTo, float _weight)
{
    const float weight = std::clamp(_weight, 0.0f, 1.0f);

    GFX::sTransform result{};

    result.position = _rFrom.position + (_rTo.position - _rFrom.position) * weight;
    result.rotation = _rFrom.rotation + (_rTo.rotation - _rFrom.rotation) * weight;
    result.scale    = _rFrom.scale    + (_rTo.scale    - _rFrom.scale)    * weight;

    return result;
}

// -------------------------------------------------------------------------------------------------------------------------
