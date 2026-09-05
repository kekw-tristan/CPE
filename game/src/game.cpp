#include "game.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "world/worldGenerator.h"
#include "world/chunk.h"

#include <algorithm>
#include <cmath>
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
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    InitMeshes();

    World::WorldGenerator::Generate(1337);

    if (LoadPlayerModel())
        BuildPlayerRenderInstances();

    m_enemyModelsLoaded = LoadEnemyModels();
    RefreshWorldRenderInstances();

    UpdateEnemyRenderInstances(0.0f);

    RebuildInstanceList();

    Platform::SetMouseCaptured(true);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
    //UpdateFreeCam(_deltaTime);

    UpdatePlayer();

    if (World::WorldGenerator::Update(m_playerController.GetPosition()))
        RefreshWorldRenderInstances();

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
    UpdateEnemyRenderInstances(_deltaTime);
    SyncProjectileRenderInstances();

}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnPrepareRender()
{
    Engine::GFX::UpdateInstanceBuffer(m_instances);

    PrepareEnemyHealthBars(Engine::GFX::GetCamera());
    Engine::GFX::UpdateHealthBars(m_healthBars);
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

    // Navigation uses immutable layout data even before an arena's chunk is loaded.
    for (const auto& definition : World::WorldGenerator::GetLayout().dungeons)
    {
        auto& dungeon = hudState.dungeons[static_cast<size_t>(definition.type)];
        const auto offset = definition.center - m_playerController.GetPosition();

        dungeon.offsetX  = offset.x();
        dungeon.offsetZ  = offset.z();
        dungeon.distance = std::sqrt(offset.x() * offset.x() + offset.z() * offset.z());
        dungeon.inArena  = std::abs(offset.x()) <= 12.5f && std::abs(offset.z()) <= 12.5f;
    }

    for (const auto& handle : m_bossHandles)
    {
        const auto* pEnemy = m_enemyManager.TryGetEnemy(handle);
        if (pEnemy == nullptr || !pEnemy->isBoss)
            continue;

        auto& dungeon = hudState.dungeons[static_cast<size_t>(pEnemy->type)];
        dungeon.defeated       = pEnemy->state == Gameplay::eEnemyState::Dead;
        dungeon.healthFraction = pEnemy->health / pEnemy->definition.maxHealth;
    }

    m_hud.Draw(hudState);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnShutdown()
{
    World::WorldGenerator::Clear();
    m_worldEnemies.clear();
    m_bossHandles.clear();
    m_worldRenderInstances.clear();

    m_enemyManager.Clear();
    m_projectileManager.Clear();
    m_enemyVisuals.clear();
    m_healthBars.clear();
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

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_thornwolf.json", m_thornwolfModel, errorMessage))
    {
        std::cerr << "Failed to load forest_thornwolf: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_thornwolf_attack.json", m_thornwolfModel, m_thornwolfAttackModel);

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_sporecap.json", m_sporecapModel, errorMessage))
    {
        std::cerr << "Failed to load forest_sporecap: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_sporecap_attack.json", m_sporecapModel, m_sporecapAttackModel);

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

void cGame::SpawnEnemies(const std::vector<World::sEnemySpawn>& _rSpawns, const std::pair<int, int>& _rChunk)
{
    for (const World::sEnemySpawn& spawn : _rSpawns)
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

        case World::sEnemyType::ForestThornwolf:
            pModel = &m_thornwolfModel;
            break;

        case World::sEnemyType::ForestSporecap:
            pModel = &m_sporecapModel;
            break;
        }

        if (pModel == nullptr)
            continue;

        sEnemyVisual visual{};
        visual.chunk = _rChunk;

        const auto key = std::make_tuple(spawn.position.x(), spawn.position.y(), spawn.position.z());
        auto [entry, inserted] = m_worldEnemies.try_emplace(key);

        if (inserted)
        {
            entry->second = m_enemyManager.Spawn(spawn.type, spawn.position, spawn.rotation, spawn.isBoss);
            if (spawn.isBoss)
                m_bossHandles.push_back(entry->second);
        }

        visual.handle = entry->second;

        m_enemyManager.SetActive(visual.handle, true);

        visual.previousPosition = m_enemyManager.TryGetEnemy(visual.handle)->position;
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

void cGame::RefreshWorldRenderInstances()
{
    const auto& chunks = World::WorldGenerator::GetLoadedChunks();
    std::unordered_set<GFX::sInstanceData*> removed;

    std::erase_if(m_worldRenderInstances, [&](const auto& _rEntry)
    {
        if (chunks.contains(_rEntry.first))
            return false;

        removed.insert(_rEntry.second.begin(), _rEntry.second.end());
        return true;
    });

    std::erase_if(m_enemyVisuals, [&](const auto& _rVisual)
    {
        if (chunks.contains(_rVisual.chunk))
            return false;

        m_enemyManager.SetActive(_rVisual.handle, false);
        for (const auto& part : _rVisual.renderParts)
            removed.insert(part.pInstance);

        return true;
    });

    if (!removed.empty())
    {
        for (auto& [mesh, instances] : m_meshInstances)
            std::erase_if(instances, [&](auto* _pInstance) { return removed.contains(_pInstance); });

        for (auto* pInstance : removed)
            m_pool.Destroy(pInstance);
    }

    for (const auto& [coordinate, chunk] : chunks)
    {
        auto [entry, inserted] = m_worldRenderInstances.try_emplace(coordinate);
        if (!inserted)
            continue;

        for (const auto& shape : chunk.scene.GetShapeInstances())
            BuildRenderInstances(shape, entry->second);

        if (m_enemyModelsLoaded)
            SpawnEnemies(chunk.spawns, coordinate);
    }

    RebuildInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance, std::vector<GFX::sInstanceData*>& _rInstances)
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
        _rInstances.push_back(pInstance);
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

void cGame::PrepareEnemyHealthBars(const GFX::cCamera& _rCamera)
{
    using namespace Engine::Math;

    m_healthBars.clear();

    float cameraPosition[4]{};
    float cameraDirection[4]{};
    _rCamera.GetPosition(cameraPosition);
    _rCamera.GetDirection(cameraDirection);

    const cVec3f position(cameraPosition[0], cameraPosition[1], cameraPosition[2]);
    const cVec3f direction(cameraDirection[0], cameraDirection[1], cameraDirection[2]);
    const float maxDistanceSquared = c_healthBarMaxDistance * c_healthBarMaxDistance;

    // Health is independent of the cached mesh transform revision.
    for (const sEnemyVisual& visual : m_enemyVisuals)
    {
        const Gameplay::sEnemy* pEnemy = m_enemyManager.TryGetEnemy(visual.handle);

        if (pEnemy == nullptr || pEnemy->state == Gameplay::eEnemyState::Dead || pEnemy->health <= 0.0f)
        {
            continue;
        }

        const float maxHealth = pEnemy->definition.maxHealth;

        if (maxHealth <= 0.0f || pEnemy->health >= maxHealth)
        {
            continue;
        }

        const float heightOffset = pEnemy->type == World::sEnemyType::ForestCrawler
            ? c_crawlerHealthBarOffset
            : pEnemy->type == World::sEnemyType::ForestThornwolf ? 2.2f
            : pEnemy->type == World::sEnemyType::ForestSporecap ? 2.7f
            : c_bruteHealthBarOffset;

        const cVec3f anchor = pEnemy->position + cVec3f(0.0f, heightOffset * pEnemy->scale, 0.0f);
        const cVec3f cameraOffset = anchor - position;

        if (cameraOffset.lengthSquared() > maxDistanceSquared || cameraOffset.dot(direction) <= 0.0f)
        {
            continue;
        }

        GFX::sHealthBarData bar{};
        bar.positionWidth[0] = anchor.x();
        bar.positionWidth[1] = anchor.y();
        bar.positionWidth[2] = anchor.z();
        bar.positionWidth[3] = c_healthBarWidth;
        bar.heightFill[0] = c_healthBarHeight;
        bar.heightFill[1] = std::clamp(pEnemy->health / maxHealth, 0.0f, 1.0f);

        m_healthBars.push_back(bar);
    }

    // Billboards do not write depth, so closer bars must be drawn last.
    const auto viewDepth = [&direction](const GFX::sHealthBarData& _rBar)
    {
        return cVec3f(_rBar.positionWidth[0], _rBar.positionWidth[1], _rBar.positionWidth[2]).dot(direction);
    };

    std::sort(m_healthBars.begin(), m_healthBars.end(), [&viewDepth](const auto& _rLeft, const auto& _rRight)
    {
        return viewDepth(_rLeft) > viewDepth(_rRight);
    });

    if (m_healthBars.size() > GFX::c_maxNumberOfHealthBars)
    {
        const auto excessCount = m_healthBars.size() - GFX::c_maxNumberOfHealthBars;
        m_healthBars.erase(m_healthBars.begin(), m_healthBars.begin() + excessCount);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateEnemyRenderInstances(float _deltaTime)
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    for (sEnemyVisual& visual : m_enemyVisuals)
    {
        const Gameplay::sEnemy* pEnemy = m_enemyManager.TryGetEnemy(visual.handle);

        if (pEnemy == nullptr)
            continue;

        const bool isAttacking = pEnemy->state == Gameplay::eEnemyState::AttackWindup || pEnemy->state == Gameplay::eEnemyState::AttackRecovery;

        const bool isThornwolf      = pEnemy->type == World::sEnemyType::ForestThornwolf;
        const bool hasWalkAnimation = isThornwolf || pEnemy->type == World::sEnemyType::ForestSporecap;
        const cVec3f displacement   = pEnemy->position - visual.previousPosition;

        visual.previousPosition = pEnemy->position;

        const float distance            = std::sqrt(displacement.x() * displacement.x() + displacement.z() * displacement.z());
        const float previousWalkWeight  = visual.walkWeight;    
        const bool  isWalking           = hasWalkAnimation && distance > 0.0001f && !isAttacking && pEnemy->state != Gameplay::eEnemyState::Dead;
        const float targetWalkWeight    = isWalking ? 1.0f : 0.0f;
        const float blendStep           = std::max(0.0f, _deltaTime) * 8.0f;

        visual.walkWeight += std::clamp(targetWalkWeight - visual.walkWeight, -blendStep, blendStep);

        // Drive the gait from distance actually travelled, including collision and retreat movement.
        constexpr float c_twoPi = 6.28318530718f;
        if (isWalking)
        {
            const float strideLength = isThornwolf ? 1.8f : 1.1f;
            const cVec3f forward(std::sin(pEnemy->rotation), 0.0f, std::cos(pEnemy->rotation));
            const float travelSign = displacement.dot(forward) < 0.0f ? -1.0f : 1.0f;
            visual.walkPhase = std::fmod(visual.walkPhase + travelSign * distance * c_twoPi / strideLength, c_twoPi);
        }

        if (visual.transformRevision == pEnemy->transformRevision && !isAttacking && !visual.wasAttacking
            && visual.walkWeight == 0.0f && previousWalkWeight == 0.0f)
            continue;

        sTransform enemyTransform{};

        enemyTransform.position = pEnemy->position;
        enemyTransform.rotation = { 0.0f, pEnemy->rotation, 0.0f };
        enemyTransform.scale    = pEnemy->state == Gameplay::eEnemyState::Dead
            ? Math::cVec3f(0.0f, 0.0f, 0.0f)
            : Math::cVec3f(pEnemy->scale, pEnemy->scale, pEnemy->scale);

        const cMatrix4x4f enemyMatrix = CreateTransformMatrix(enemyTransform);

        const sShapeModelDesc& attackModel = pEnemy->type == World::sEnemyType::ForestCrawler ? m_enemy03AttackModel
            : pEnemy->type == World::sEnemyType::ForestThornwolf ? m_thornwolfAttackModel
            : pEnemy->type == World::sEnemyType::ForestSporecap ? m_sporecapAttackModel
            : m_enemy04AttackModel;

        for (size_t partIndex = 0; partIndex < visual.renderParts.size(); ++partIndex)
        {
            sEnemyRenderPart& renderPart    = visual.renderParts[partIndex];
            sTransform partTransform = attackModel.shapes.empty() ? renderPart.transform : InterpolateTransform(renderPart.transform, attackModel.shapes[partIndex].transform, pEnemy->attackPoseWeight);

            const float walkWeight = visual.walkWeight * (1.0f - pEnemy->attackPoseWeight);
            if (hasWalkAnimation && walkWeight > 0.0f)
            {
                // Both forest models keep their feet below y=0.5; use the rest pose to identify legs.
                const cVec3f& restPosition = renderPart.transform.position;
                const bool isLeg = restPosition.y() < 0.5f;
                const float bounce = (1.0f - std::cos(2.0f * visual.walkPhase)) * 0.025f * walkWeight;
                if (isThornwolf)
                    partTransform.position += cVec3f(0.0f, bounce, 0.0f);
                if (isLeg)
                {
                    // The wolf trots with diagonal pairs; the mushroom alternates its two feet.
                    const bool oppositePhase = isThornwolf
                        ? (restPosition.x() < 0.0f) != (restPosition.z() < 0.0f)
                        : restPosition.x() < 0.0f;
                    const float step = std::sin(visual.walkPhase) * (oppositePhase ? -1.0f : 1.0f);
                    if (isThornwolf)
                    {
                        const float angle = step * 0.55f * walkWeight;
                        const float hipOffset = renderPart.transform.scale.y() * 0.5f;
                        partTransform.rotation += cVec3f(angle, 0.0f, 0.0f);
                        partTransform.position += cVec3f(0.0f, hipOffset * (1.0f - std::cos(angle)), -hipOffset * std::sin(angle));
                    }
                    else
                    {
                        partTransform.position += cVec3f(0.0f, std::max(0.0f, step) * 0.16f * walkWeight, step * 0.2f * walkWeight);
                    }
                }
                else
                {
                    if (!isThornwolf)
                    {
                        partTransform.position += cVec3f(0.0f, bounce, 0.0f);
                        const float sway = std::sin(visual.walkPhase) * 0.045f * walkWeight;
                        partTransform.position += cVec3f(sway * restPosition.y(), 0.0f, 0.0f);
                        partTransform.rotation += cVec3f(0.0f, 0.0f, -sway);
                    }
                }
            }
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

            const bool isSpore = projectile.type == Gameplay::eProjectileType::EnemySpore;

            pInstance->color = isPlayerSpell
                ? std::array<float, 4>{ 0.5f, 0.15f, 1.0f, 1.0f }
                : isSpore ? std::array<float, 4>{ 0.48f, 0.16f, 0.22f, 1.0f }
                : std::array<float, 4>{ 0.35f, 1.0f, 0.18f, 1.0f };

            const sShapeModelDesc& materialModel = isPlayerSpell ? m_playerModel : isSpore ? m_sporecapModel : m_enemy03Model;
            const size_t materialSlot = isPlayerSpell ? 3 : isSpore ? 0 : 2;
            pInstance->materialIndex = materialModel.materialIndices.size() > materialSlot ? materialModel.materialIndices[materialSlot] : 0;

            const MeshHandle mesh = isPlayerSpell || isSpore ? m_sphereMesh : m_coneMesh;

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
        else if (projectile.type == Gameplay::eProjectileType::EnemySpore)
        {
            const float pulse = std::sin(projectile.lifetime * 9.0f);
            transform.rotation = { projectile.lifetime * 2.0f, projectile.lifetime * 1.5f, 0.0f };
            transform.scale = { 0.48f + pulse * 0.04f, 0.42f - pulse * 0.04f, 0.48f + pulse * 0.04f };
            const float tint = (pulse + 1.0f) * 0.5f;
            visual->pInstance->color = { 0.48f + tint * 0.20f, 0.16f + tint * 0.35f, 0.22f - tint * 0.10f, 1.0f };
        }
        else
        {
            transform.rotation = { 1.5707963f, std::atan2(projectile.direction.x(), projectile.direction.z()), 0.0f };
            transform.scale = { 0.14f, 0.65f, 0.14f };
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
