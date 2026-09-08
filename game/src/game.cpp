#include "game.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelLights.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "world/worldGenerator.h"
#include "world/chunk.h"
#include "world/terrainHeight.h"

#include "spells/spellManager.h"

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
    , m_cameraPitch(-10.f)
    , m_meshInstances()
    , m_inventory()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    InitMeshes();

    InitNightSky();

    World::WorldGenerator::Generate(1337);

    if (LoadPlayerModel())
        BuildPlayerRenderInstances();

    m_enemyModelsLoaded = LoadEnemyModels();
    RefreshWorldRenderInstances();

    UpdateEnemyRenderInstances(0.0f);

    RebuildInstanceList();

    Platform::SetMouseCaptured(true);

    // inventory test
    m_inventory.AddItem(Gameplay::sItemId::HealthPotion, 5);
    m_inventory.AddItem(Gameplay::sItemId::ManaPotion, 3);

    m_inventory.AddItem(Gameplay::sItemId::ForestHelmet);
    m_inventory.AddItem(Gameplay::sItemId::ForestChest);
    m_inventory.AddItem(Gameplay::sItemId::ForestRing);

    BeginRun();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
    UpdateInventoryInput();

    const bool augmentSelectionPending = m_runState.HasPendingAugmentSelection();

    if (!augmentSelectionPending)
        m_runState.Update(_deltaTime);

    constexpr int c_leftAltKey = 342;
    constexpr int c_rightAltKey = 346;
    const bool altDown = Engine::Platform::IsKeyDown(c_leftAltKey) || Engine::Platform::IsKeyDown(c_rightAltKey);

    if (altDown && !m_altWasDown)
    {
        m_mouseReleased = !m_mouseReleased;
        Engine::Platform::SetMouseCaptured(!m_inventoryOpen && !m_mouseReleased && !augmentSelectionPending);
    }
    m_altWasDown = altDown;

    //UpdateFreeCam(_deltaTime);

    if (!m_inventoryOpen && !augmentSelectionPending)
    {
        UpdatePlayer();

        m_playerController.Update(_deltaTime);
        UpdateThirdPersonCamera(_deltaTime);
        UpdatePlayerSpell(_deltaTime);
    }

    if (!augmentSelectionPending)
    {
        if (World::WorldGenerator::Update(m_playerController.GetPosition()))
            RefreshWorldRenderInstances();


        Gameplay::sEnemyUpdateContext enemyContext{};
        enemyContext.deltaTime      = _deltaTime;
        enemyContext.playerPosition = m_playerController.GetPosition();

        m_enemyManager.Update(enemyContext, m_projectileManager);
        m_projectileManager.Update(_deltaTime, enemyContext.playerPosition, m_enemyManager);

        for (const Gameplay::sEnemyDeathEvent& deathEvent : m_enemyManager.GetDeathEvents())
        {
            const uint32_t experience = deathEvent.isBoss ? c_bossExperience : c_regularEnemyExperience;
            ApplyLevelUpRewards(m_runState.GrantExperience(experience));

            if (!deathEvent.isBoss || deathEvent.bossId == World::sBossId::Undefined)
                continue;

            const Gameplay::SpellManager::sBossDefinition& boss = Gameplay::SpellManager::GetBoss(deathEvent.bossId);

            if (!m_runState.GrantSpell(boss.spellReward))
                continue;

            const Gameplay::sSpellDefinition& spell = Gameplay::SpellManager::GetSpell(boss.spellReward);
            m_inventory.AddItem(spell.inventoryItem);
        }

        m_enemyManager.ClearDeathEvents();

        const float receivedDamage = m_enemyManager.ConsumePlayerDamage() + m_projectileManager.ConsumePlayerDamage();
        if (receivedDamage > 0.0f)
        {
            m_playerHealth = std::max(0.0f, m_playerHealth - receivedDamage);
            std::cout << "Player health: " << m_playerHealth << '\n';
        }

        if (m_runState.HasPendingAugmentSelection())
            Engine::Platform::SetMouseCaptured(false);
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

    hudState.health                                 = m_playerHealth;
    hudState.maxHealth                              = m_playerMaxHealth;
    hudState.mana                                   = m_playerMana;
    hudState.maxMana                                = m_playerMaxMana;
    hudState.xp                                     = m_runState.GetExperience();
    hudState.xpToNextLevel                          = m_runState.GetExperienceToNextLevel();
    hudState.level                                  = m_runState.GetLevel();
    hudState.inventory.visible                      = m_inventoryOpen;
    hudState.augmentSelection.visible               = m_runState.HasPendingAugmentSelection();
    hudState.augmentSelection.selectionsRemaining   = m_runState.GetPendingAugmentSelections();

    const auto& augmentChoices = m_runState.GetAugmentChoices();
    for (size_t choiceIndex = 0; choiceIndex < augmentChoices.size(); ++choiceIndex)
    {
        const Gameplay::sSpellAugment::Enum augment = augmentChoices[choiceIndex];
        hudState.augmentSelection.choices[choiceIndex] = augment;
        hudState.augmentSelection.stackCounts[choiceIndex] = m_runState.GetAugmentCount(augment);
    }

    for (size_t slotIndex = 0; slotIndex < hudState.spellCooldowns.size(); ++slotIndex)
    {
        const Gameplay::cSpellInstance* pSpell = m_runState.GetSpellInSlot(slotIndex);
        if (pSpell == nullptr)
            continue;

        hudState.spellCooldowns[slotIndex]          = pSpell->GetCooldownRemaining();
        hudState.spellCooldownDurations[slotIndex]  = pSpell->GetSpellStats().cooldown;
        hudState.spellManaCosts[slotIndex]          = Gameplay::SpellManager::GetSpell(pSpell->GetSpellId()).manaCost;
        hudState.anySpellOnCooldown                 = hudState.anySpellOnCooldown || pSpell->IsOnCooldown();
    }

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

    // quest
    for (const auto& handle : m_bossHandles)
    {
        const auto* pEnemy = m_enemyManager.TryGetEnemy(handle);
        if (pEnemy == nullptr || !pEnemy->isBoss)
            continue;

        auto& dungeon = hudState.dungeons[static_cast<size_t>(pEnemy->type)];
        dungeon.defeated       = pEnemy->state == Gameplay::eEnemyState::Dead;
        dungeon.healthFraction = pEnemy->health / pEnemy->definition.maxHealth;
    }

    const auto& inventorySlots = m_inventory.GetInventorySlots();

    // inventory
    for (size_t i = 0; i < inventorySlots.size(); ++i)
    {
        hudState.inventory.inventorySlots[i].item = inventorySlots[i].item;
        hudState.inventory.inventorySlots[i].amount = inventorySlots[i].amount;
    }

    const auto& usableSlots = m_inventory.GetUsableSlots();

    for (size_t i = 0; i < usableSlots.size(); ++i)
    {
        hudState.inventory.usableSlots[i].item = usableSlots[i].item;
        hudState.inventory.usableSlots[i].amount = usableSlots[i].amount;
    }

    const auto& spellSlots = m_inventory.GetSpellSlots();

    for (size_t i = 0; i < spellSlots.size(); ++i)
    {
        hudState.inventory.spellSlots[i].item = spellSlots[i].item;
        hudState.inventory.spellSlots[i].amount = spellSlots[i].amount;
    }

    const auto& armorSlots = m_inventory.GetArmorSlots();

    for (size_t i = 0; i < armorSlots.size(); ++i)
    {
        hudState.inventory.armorSlots[i].item = armorSlots[i].item;
        hudState.inventory.armorSlots[i].amount = armorSlots[i].amount;
    }

    m_hud.Draw(hudState);

    Gameplay::sSpellAugment::Enum selectedAugment = Gameplay::sSpellAugment::Undefined;
    if (m_hud.ConsumeAugmentSelection(selectedAugment) && m_runState.SelectAugment(selectedAugment))
        Engine::Platform::SetMouseCaptured(!m_inventoryOpen && !m_mouseReleased && !m_runState.HasPendingAugmentSelection());

    UI::eInventoryAction inventoryAction = UI::eInventoryAction::MoveItem;
    size_t sourceInventorySlot = 0;
    size_t destinationInventorySlot = 0;

    if (m_hud.ConsumeInventoryAction(inventoryAction, sourceInventorySlot, destinationInventorySlot))
    {
        switch (inventoryAction)
        {
            case UI::eInventoryAction::MoveItem:
                m_inventory.MoveItem(sourceInventorySlot, destinationInventorySlot);
                break;

            case UI::eInventoryAction::EquipArmor:
                m_inventory.EquipArmor(sourceInventorySlot);
                break;

            case UI::eInventoryAction::EquipUsable:
                m_inventory.EquipUsable(sourceInventorySlot, destinationInventorySlot);
                break;

            case UI::eInventoryAction::EquipSpell:
                m_inventory.EquipSpell(sourceInventorySlot, destinationInventorySlot);
                break;

            case UI::eInventoryAction::MoveSpell:
                m_inventory.MoveSpell(sourceInventorySlot, destinationInventorySlot);
                break;

            case UI::eInventoryAction::UnequipArmor:
                m_inventory.UnequipArmor(
                    static_cast<Gameplay::sArmorSlot::Enum>(sourceInventorySlot),
                    destinationInventorySlot);
                break;

            case UI::eInventoryAction::UnequipUsable:
                m_inventory.UnequipUsable(sourceInventorySlot, destinationInventorySlot);
                break;

            case UI::eInventoryAction::UnequipSpell:
                m_inventory.UnequipSpell(sourceInventorySlot, destinationInventorySlot);
                break;
        }

        SyncSpellLoadoutFromInventory();
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnShutdown()
{
    World::WorldGenerator::Clear();
    m_worldEnemies.clear();
    m_bossHandles.clear();

    for (auto& [coordinate, instances] : m_worldRenderInstances)
        GFX::ShapeModelLights::Destroy(instances.lightHandles);

    m_worldRenderInstances.clear();

    m_enemyManager.Clear();
    m_projectileManager.Clear();

    for (sEnemyVisual& visual : m_enemyVisuals)
        GFX::ShapeModelLights::Destroy(visual.lightHandles);

    m_enemyVisuals.clear();
    m_healthBars.clear();

    for (const sProjectileVisual& visual : m_projectileVisuals)
        GFX::LightManager::DestroyLight(visual.light);

    m_projectileVisuals.clear();
    GFX::ShapeModelLights::Destroy(m_playerLightHandles);
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

    m_torusMesh             = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Torus));
    m_crystalMesh           = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Crystal));
    m_beveledCubeMesh       = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::BeveledCube));
    m_frustumMesh           = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Frustum));
    m_wedgeMesh             = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Wedge));
    m_triangularPrismMesh   = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::TriangularPrism));
    m_icoSphereMesh         = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::IcoSphere));
    m_rockMesh              = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Rock));
    m_grassBladeMesh        = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::GrassBlade));
    m_capsuleMesh           = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Capsule));
    m_archMesh              = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Arch));
    m_extrudedPolygonMesh   = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::ExtrudedPolygon));
    m_discMesh              = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Disc));
    m_arcMesh               = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Arc));

    SubmitMesh(m_planeMesh);
    SubmitMesh(m_chunkPlaneMesh);
    SubmitMesh(m_cubeMesh);
    SubmitMesh(m_pyramidMesh);
    SubmitMesh(m_sphereMesh);
    SubmitMesh(m_cylinderMesh);
    SubmitMesh(m_coneMesh);
    SubmitMesh(m_torusMesh);
    SubmitMesh(m_crystalMesh);
    SubmitMesh(m_beveledCubeMesh);
    SubmitMesh(m_frustumMesh);
    SubmitMesh(m_wedgeMesh);
    SubmitMesh(m_triangularPrismMesh);
    SubmitMesh(m_icoSphereMesh);
    SubmitMesh(m_rockMesh);
    SubmitMesh(m_grassBladeMesh);
    SubmitMesh(m_capsuleMesh);
    SubmitMesh(m_archMesh);
    SubmitMesh(m_extrudedPolygonMesh);
    SubmitMesh(m_discMesh);
    SubmitMesh(m_arcMesh);

    sMaterial playerSphereMaterial{};

    playerSphereMaterial.roughness        = 0.18f;
    playerSphereMaterial.lightWrap        = 1.0f;
    playerSphereMaterial.ambientStrength  = 0.0f;
    playerSphereMaterial.emissiveColor    = { 0.5f, 0.15f, 1.0f };
    playerSphereMaterial.emissiveStrength = 4.0f;

    m_playerSphereMaterial = MaterialManager::CreateMaterial(playerSphereMaterial);

    sLight directionalLight0{};
    
    directionalLight0.type          = sLightType::Directional;
    directionalLight0.color         = { 0.26f, 0.32f, 0.35f };
    directionalLight0.intensity     = 0.7f;
    directionalLight0.direction     = { -0.5f, -0.5f, -0.3f };
    directionalLight0.castsShadow   = true;
    
    LightManager::CreateLight(directionalLight0);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::InitNightSky()
{
    using namespace Engine::GFX;

    // An inward-facing cube follows the camera in the shader; the application owns its GPU mesh.
    sMeshData skyMesh = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    skyMesh.pDebugName = "Geometric night sky";

    for (size_t i = 0; i < skyMesh.indices.size(); i += 3)
        std::swap(skyMesh.indices[i + 1], skyMesh.indices[i + 2]);

    MeshHandle mesh = CreateMesh(skyMesh);
    SubmitMesh(mesh);

    sInstanceData* pInstance = m_pool.Create();
    pInstance->worldMatrix = Engine::Math::cMatrix4x4f::identity();
    pInstance->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pInstance->materialIndex = -1;
    pInstance->instanceFlags = sInstanceFlags::InstanceFlagSky;
    m_meshInstances[mesh].push_back(pInstance);
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

    if (_rPoseModel.lights.size() != _rBaseModel.lights.size())
    {
        std::cerr << "Pose model has a different light count: " << _pFilePath << '\n';
        _rPoseModel = {};
        return false;
    }

    for (size_t lightIndex = 0; lightIndex < _rBaseModel.lights.size(); ++lightIndex)
    {
        const GFX::sShapeLightDesc& baseLight = _rBaseModel.lights[lightIndex];
        const GFX::sShapeLightDesc& poseLight = _rPoseModel.lights[lightIndex];

        if (poseLight.name != baseLight.name || poseLight.type != baseLight.type)
        {
            std::cerr << "Pose model has a different light at index " << lightIndex << ": " << _pFilePath << '\n';
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
        visual.pModel = pModel;

        const auto key = std::make_tuple(spawn.position.x(), spawn.position.y(), spawn.position.z());
        auto [entry, inserted] = m_worldEnemies.try_emplace(key);

        if (inserted)
        {
            entry->second = m_enemyManager.Spawn(spawn.type, spawn.position, spawn.rotation, spawn.isBoss, spawn.bossId);
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
            pInstance->instanceFlags |= GFX::sInstanceFlags::InstanceFlagPreserveAtDistance;

            GFX::MeshHandle mesh = GetMesh(part.meshType);

            m_meshInstances[mesh].push_back(pInstance);

            sEnemyRenderPart renderPart{};

            renderPart.pInstance = pInstance;
            renderPart.transform = part.transform;

            visual.renderParts.push_back(renderPart);
        }

        const Gameplay::sEnemy* pEnemy = m_enemyManager.TryGetEnemy(visual.handle);

        if (pEnemy != nullptr)
        {
            GFX::sTransform enemyTransform{};
            enemyTransform.position = pEnemy->position;
            enemyTransform.rotation = { 0.0f, pEnemy->rotation, 0.0f };
            enemyTransform.scale    = { pEnemy->scale, pEnemy->scale, pEnemy->scale };

            GFX::ShapeModelLights::Create(*pModel, enemyTransform, visual.lightHandles);
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

    std::erase_if(m_worldRenderInstances, [&](auto& _rEntry)
    {
        if (chunks.contains(_rEntry.first))
            return false;

        removed.insert(_rEntry.second.renderInstances.begin(), _rEntry.second.renderInstances.end());
        GFX::ShapeModelLights::Destroy(_rEntry.second.lightHandles);
        return true;
    });

    std::erase_if(m_enemyVisuals, [&](auto& _rVisual)
    {
        if (chunks.contains(_rVisual.chunk))
            return false;

        m_enemyManager.SetActive(_rVisual.handle, false);
        for (const auto& part : _rVisual.renderParts)
            removed.insert(part.pInstance);

        GFX::ShapeModelLights::Destroy(_rVisual.lightHandles);

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

void cGame::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance, sWorldRenderInstances& _rInstances)
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
        pInstance->instanceFlags |= sInstanceFlags::InstanceFlagPreserveAtDistance;

        MeshHandle mesh = GetMesh(part.meshType);

        if (part.meshType == sMeshTypes::ChunkPlane)
        {
            pInstance->instanceFlags |= sInstanceFlags::InstanceFlagTerrain;
        }

        m_meshInstances[mesh].push_back(pInstance);
        _rInstances.renderInstances.push_back(pInstance);
    }

    std::vector<LightHandle> lightHandles;
    ShapeModelLights::Create(model, _rShapeInstance.transform, lightHandles);
    _rInstances.lightHandles.insert(_rInstances.lightHandles.end(), lightHandles.begin(), lightHandles.end());
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

    sTransform playerTransform{};
    playerTransform.position = m_playerController.GetPosition();
    playerTransform.rotation = { 0.0f, m_playerYaw, 0.0f };
    playerTransform.scale    = { 1.0f, 1.0f, 1.0f };

    ShapeModelLights::Create(m_playerModel, playerTransform, m_playerLightHandles);
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
    constexpr int c_leftMouseButton = 0;
    constexpr int c_rightMouseButton = 1;

    const std::array<int, 4> c_spellKeys = { 'Q', 'E', 'R', 'F' };
    std::array<bool, c_spellKeys.size()> spellKeysPressed{};

    for (size_t keyIndex = 0; keyIndex < c_spellKeys.size(); ++keyIndex)
    {
        const bool keyDown = Engine::Platform::IsKeyDown(c_spellKeys[keyIndex]);
        spellKeysPressed[keyIndex] = keyDown && !m_spellKeysWasDown[keyIndex];
        m_spellKeysWasDown[keyIndex] = keyDown;
    }

    m_playerAttackTime = std::max(0.0f, m_playerAttackTime - _deltaTime);

    if (m_mouseReleased)
        return;

    size_t spellSlot = Gameplay::cRunState::c_numberOfSpellSlots;

    if (Engine::Platform::WasMouseButtonPressed(c_leftMouseButton))
        spellSlot = 0;
    else if (Engine::Platform::WasMouseButtonPressed(c_rightMouseButton))
        spellSlot = 1;
    else if (spellKeysPressed[0])
        spellSlot = 2;
    else if (spellKeysPressed[1])
        spellSlot = 3;
    else if (spellKeysPressed[2])
        spellSlot = 4;
    else if (spellKeysPressed[3])
        spellSlot = 5;

    if (spellSlot >= Gameplay::cRunState::c_numberOfSpellSlots)
        return;

    const Gameplay::cSpellInstance* pSpell = m_runState.GetSpellInSlot(spellSlot);

    if (pSpell == nullptr)
        return;

    if (pSpell->IsOnCooldown())
        return;

    const Gameplay::sSpellDefinition& spellDefinition = Gameplay::SpellManager::GetSpell(pSpell->GetSpellId());

    if (spellDefinition.castType != Gameplay::sSpellCastType::Projectile
        && spellDefinition.castType != Gameplay::sSpellCastType::ConeProjectile
        && spellDefinition.castType != Gameplay::sSpellCastType::SporeProjectile)
        return;

    if (m_playerMana < spellDefinition.manaCost)
        return;

    const Gameplay::sSpellStats& spellStats = pSpell->GetSpellStats();

    using Engine::Math::cVec3f;

    float cameraDirection[4];
    float cameraPosition[4];
    Engine::GFX::GetCamera().GetDirection(cameraDirection);
    Engine::GFX::GetCamera().GetPosition(cameraPosition);

    const cVec3f viewDirection = cVec3f(cameraDirection[0], cameraDirection[1], cameraDirection[2]).normalized();
    const cVec3f viewPosition(cameraPosition[0], cameraPosition[1], cameraPosition[2]);
    const cVec3f castPosition = m_playerController.GetPosition() + cVec3f(0.0f, 1.25f, 0.0f);

    float aimDistance = cVec3f::distance(viewPosition, castPosition) + spellStats.projectileSpeed * spellStats.duration;

    // Converge on the nearest enemy under the reticle, compensating for the shoulder camera.
    aimDistance = m_enemyManager.FindAimDistance(viewPosition, viewDirection, aimDistance);

    // Terrain in front of that target takes precedence. This runs only when casting.
    for (float distance = 0.25f; distance < aimDistance; distance += 0.25f)
    {
        const cVec3f sample = viewPosition + viewDirection * distance;
        if (sample.y() <= World::GetTerrainSurfaceHeight(sample.x(), sample.z()))
        {
            aimDistance = distance;
            break;
        }
    }

    const cVec3f aimPosition = viewPosition + viewDirection * aimDistance;
    const cVec3f direction = (aimPosition - castPosition).normalized();

    if (direction.isZero() || direction.dot(viewDirection) <= 0.0f)
        return;

    m_playerYaw = std::atan2(direction.x(), direction.z());

    const float centerProjectile = 0.5f * static_cast<float>(spellStats.projectileCount - 1);

    for (int projectileIndex = 0; projectileIndex < spellStats.projectileCount; ++projectileIndex)
    {
        const float angle = (static_cast<float>(projectileIndex) - centerProjectile) * 0.12f;
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);

        Gameplay::sProjectileSpawnDesc projectile{};
        projectile.position = castPosition;
        projectile.direction = cVec3f(
            direction.x() * cosine + direction.z() * sine,
            direction.y(),
            -direction.x() * sine + direction.z() * cosine).normalized();
        projectile.speed = spellStats.projectileSpeed;
        projectile.damage = spellStats.damage;
        projectile.lifetime = spellStats.duration;
        projectile.radius = spellStats.projectileRadius;
        projectile.isAreaOfEffect = spellDefinition.castType == Gameplay::sSpellCastType::SporeProjectile;
        projectile.pierces = spellStats.pierceCount;

        switch (spellDefinition.castType)
        {
            case Gameplay::sSpellCastType::Projectile:
                m_projectileManager.SpawnPlayerSphere(projectile);
                break;

            case Gameplay::sSpellCastType::ConeProjectile:
                m_projectileManager.SpawnPlayerCone(projectile);
                break;

            case Gameplay::sSpellCastType::SporeProjectile:
                m_projectileManager.SpawnPlayerSpore(projectile);
                break;
        }
    }

    m_playerMana = std::max(0.0f, m_playerMana - spellDefinition.manaCost);
    m_runState.StartSpellCooldown(spellSlot);
    m_playerAttackTime = 0.4f;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BeginRun()
{
    m_runState.Begin();
    m_inventory.ClearSpells();
    m_playerMaxHealth = c_playerBaseMaxHealth;
    m_playerHealth = m_playerMaxHealth;
    m_playerMaxMana = c_playerBaseMaxMana;
    m_playerMana = m_playerMaxMana;

    if (!m_runState.GrantSpell(Gameplay::sSpellId::Fireball))
        return;

    m_runState.SetSpellSlot(0, Gameplay::sSpellId::Fireball);

    const Gameplay::sSpellDefinition& fireball = Gameplay::SpellManager::GetSpell(Gameplay::sSpellId::Fireball);
    if (!m_inventory.AddItem(fireball.inventoryItem))
        return;

    const auto& inventorySlots = m_inventory.GetInventorySlots();
    for (size_t inventorySlot = 0; inventorySlot < inventorySlots.size(); ++inventorySlot)
    {
        if (inventorySlots[inventorySlot].item != fireball.inventoryItem)
            continue;

        m_inventory.EquipSpell(inventorySlot, 0);
        break;
    }

    constexpr std::array<Gameplay::sSpellId::Enum, 2> c_additionalStarterSpells =
    {
        Gameplay::sSpellId::StoneShard,
        Gameplay::sSpellId::SporeOrb
    };

    for (Gameplay::sSpellId::Enum spellId : c_additionalStarterSpells)
    {
        if (!m_runState.GrantSpell(spellId))
            continue;

        m_inventory.AddItem(Gameplay::SpellManager::GetSpell(spellId).inventoryItem);
    }

    SyncSpellLoadoutFromInventory();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ApplyLevelUpRewards(uint32_t _levelUps)
{
    if (_levelUps == 0)
        return;

    const float healthBonus = c_levelUpHealthBonus * static_cast<float>(_levelUps);
    const float manaBonus = c_levelUpManaBonus * static_cast<float>(_levelUps);

    m_playerMaxHealth += healthBonus;
    m_playerHealth = std::min(m_playerMaxHealth, m_playerHealth + healthBonus);
    m_playerMaxMana += manaBonus;
    m_playerMana = std::min(m_playerMaxMana, m_playerMana + manaBonus);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::SyncSpellLoadoutFromInventory()
{
    const auto& spellSlots = m_inventory.GetSpellSlots();

    for (size_t slotIndex = 0; slotIndex < spellSlots.size(); ++slotIndex)
    {
        const Gameplay::sSpellId::Enum spellId = Gameplay::SpellManager::GetSpellId(spellSlots[slotIndex].item);
        m_runState.SetSpellSlot(slotIndex, spellId);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateInventoryInput()
{
    constexpr int c_escapeKey = 256;

    const bool inventoryKeyDown = Engine::Platform::IsKeyDown('I');
    const bool escapeKeyDown = Engine::Platform::IsKeyDown(c_escapeKey);

    if (escapeKeyDown && !m_escapeKeyWasDown && m_inventoryOpen)
    {
        m_inventoryOpen = false;

        Engine::Platform::SetMouseCaptured(!m_mouseReleased);
    }
    else if (inventoryKeyDown && !m_inventoryKeyWasDown)
    {
        m_inventoryOpen = !m_inventoryOpen;

        Engine::Platform::SetMouseCaptured(!m_inventoryOpen && !m_mouseReleased);
    }

    m_inventoryKeyWasDown = inventoryKeyDown;
    m_escapeKeyWasDown = escapeKeyDown;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayerRenderInstances()
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    if (m_playerRenderParts.empty() && m_playerModel.lights.empty())
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

    ShapeModelLights::Update(m_playerModel, m_playerAttackModel, attackWeight, playerTransform, m_playerLightHandles);
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

        if (visual.pModel != nullptr)
            ShapeModelLights::Update(*visual.pModel, attackModel, pEnemy->attackPoseWeight, enemyTransform, visual.lightHandles);

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
            const bool isPlayerSpell = projectile.type == Gameplay::eProjectileType::PlayerSphere
                || projectile.type == Gameplay::eProjectileType::PlayerCone
                || projectile.type == Gameplay::eProjectileType::PlayerSpore;
            const bool isPlayerSpore = projectile.type == Gameplay::eProjectileType::PlayerSpore;
            const bool isSpore = isPlayerSpore || projectile.type == Gameplay::eProjectileType::EnemySpore;

            pInstance->color = isPlayerSpore
                ? std::array<float, 4>{ 0.35f, 0.95f, 0.25f, 1.0f }
                : isPlayerSpell
                    ? projectile.type == Gameplay::eProjectileType::PlayerCone
                        ? std::array<float, 4>{ 0.95f, 0.52f, 0.12f, 1.0f }
                        : std::array<float, 4>{ 0.5f, 0.15f, 1.0f, 1.0f }
                    : isSpore ? std::array<float, 4>{ 0.48f, 0.16f, 0.22f, 1.0f }
                    : std::array<float, 4>{ 0.35f, 1.0f, 0.18f, 1.0f };

            if (isPlayerSpell)
            {
                pInstance->materialIndex = m_playerSphereMaterial;
            }
            else
            {
                const sShapeModelDesc& materialModel = isSpore ? m_sporecapModel : m_enemy03Model;
                const size_t materialSlot = isSpore ? 0 : 2;
                pInstance->materialIndex = materialModel.materialIndices.size() > materialSlot ? materialModel.materialIndices[materialSlot] : 0;
            }

            const MeshHandle mesh = projectile.type == Gameplay::eProjectileType::PlayerCone
                || projectile.type == Gameplay::eProjectileType::EnemyCone
                ? m_coneMesh
                : m_sphereMesh;

            m_meshInstances[mesh].push_back(pInstance);

            sProjectileVisual projectileVisual{};
            projectileVisual.id        = projectile.id;
            projectileVisual.pInstance = pInstance;
            projectileVisual.mesh      = mesh;

            if (isPlayerSpell)
            {
                sLight light{};
                light.type      = sLightType::Point;
                light.color     = isPlayerSpore
                    ? Math::cVec3f(0.35f, 0.95f, 0.25f)
                    : projectile.type == Gameplay::eProjectileType::PlayerCone
                        ? Math::cVec3f(0.95f, 0.52f, 0.12f)
                        : Math::cVec3f(0.5f, 0.15f, 1.0f);
                light.intensity = 10.0f;
                light.position  = projectile.position;
                light.radius    = 4.0f;

                projectileVisual.light = LightManager::CreateLight(light);
            }

            m_projectileVisuals.push_back(projectileVisual);

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
        else if (projectile.type == Gameplay::eProjectileType::PlayerSpore
            || projectile.type == Gameplay::eProjectileType::EnemySpore)
        {
            const float pulse = std::sin(projectile.lifetime * 9.0f);
            transform.rotation = { projectile.lifetime * 2.0f, projectile.lifetime * 1.5f, 0.0f };
            const float scale = projectile.type == Gameplay::eProjectileType::PlayerSpore ? 0.18f + projectile.radius * 0.18f : 0.48f;
            transform.scale = { scale + pulse * 0.04f, scale - pulse * 0.04f, scale + pulse * 0.04f };

            if (projectile.type == Gameplay::eProjectileType::EnemySpore)
            {
                const float tint = (pulse + 1.0f) * 0.5f;
                visual->pInstance->color = { 0.48f + tint * 0.20f, 0.16f + tint * 0.35f, 0.22f - tint * 0.10f, 1.0f };
            }
        }
        else
        {
            const float horizontalLength = std::sqrt(projectile.direction.x() * projectile.direction.x()
                + projectile.direction.z() * projectile.direction.z());
            const float pitch = std::atan2(horizontalLength, projectile.direction.y());
            const float yaw = std::atan2(projectile.direction.x(), projectile.direction.z());

            transform.rotation = { pitch, yaw, 0.0f };
            transform.scale = { 0.14f, 0.65f, 0.14f };
        }

        visual->pInstance->worldMatrix = CreateTransformMatrix(transform);

        if (sLight* pLight = LightManager::TryGetLight(visual->light))
            pLight->position = projectile.position;
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
        LightManager::DestroyLight(visual->light);
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

    constexpr float c_mouseSensitivity  = 0.09f;
    constexpr float c_minCameraDistance = 2.5f;
    constexpr float c_maxCameraDistance = 10.0f;
    constexpr float c_zoomStep          = 0.75f;
    constexpr float c_targetHeight      = 1.8f;
    constexpr float c_shoulderOffset    = 0.75f;
    constexpr float c_minPitch          = -85.0f;
    // Keep the distorted zenith outside the 60-degree vertical field of view.
    constexpr float c_maxPitch          = 30.0f;
    constexpr float c_groundClearance   = 0.35f;
    constexpr int   c_cameraSteps       = 50;

    GFX::cCamera& rCamera = GFX::GetCamera();

    const float mouseDeltaX = m_mouseReleased ? 0.0f : Platform::GetMouseDeltaX();
    const float mouseDeltaY = m_mouseReleased ? 0.0f : Platform::GetMouseDeltaY();
    const float mouseWheelDelta = m_mouseReleased ? 0.0f : Platform::GetMouseWheelDelta();

    m_cameraDistance = std::clamp(m_cameraDistance - mouseWheelDelta * c_zoomStep,
                                  c_minCameraDistance,
                                  c_maxCameraDistance);

    rCamera.AddYaw(mouseDeltaX * c_mouseSensitivity);

    const float pitchChange = -mouseDeltaY * c_mouseSensitivity;
    const float newPitch    = std::clamp(m_cameraPitch + pitchChange, c_minPitch, c_maxPitch);

    float previousDirection[4];
    rCamera.GetDirection(previousDirection);

    const float yawDegrees = std::atan2(previousDirection[2], previousDirection[0]) * 180.0f / 3.14159265358979323846f;
    rCamera.SetRotation(yawDegrees, newPitch);
    m_cameraPitch = newPitch;

    float direction[4];
    rCamera.GetDirection(direction);

    Math::cVec3f cameraDirection(direction[0], direction[1], direction[2]);
    cameraDirection.normalize();

    const Math::cVec3f cameraRight = cameraDirection.cross(Math::cVec3f(0.0f, 1.0f, 0.0f)).normalized();
    const Math::cVec3f targetPosition = m_playerController.GetPosition() + Math::cVec3f(0.0f, c_targetHeight, 0.0f)
        + cameraRight * c_shoulderOffset;
    Math::cVec3f cameraPosition = targetPosition;

    // Stop the camera arm before it enters the terrain, including on hills.
    for (int step = 1; step <= c_cameraSteps; ++step)
    {
        const float distance = m_cameraDistance * static_cast<float>(step) / static_cast<float>(c_cameraSteps);
        const Math::cVec3f candidatePosition = targetPosition - cameraDirection * distance;
        const float groundHeight = World::GetTerrainSurfaceHeight(candidatePosition.x(), candidatePosition.z());

        if (candidatePosition.y() < groundHeight + c_groundClearance)
            break;

        cameraPosition = candidatePosition;
    }

    const float minimumHeight = World::GetTerrainSurfaceHeight(cameraPosition.x(), cameraPosition.z()) + c_groundClearance;
    cameraPosition = Math::cVec3f(cameraPosition.x(), std::max(cameraPosition.y(), minimumHeight), cameraPosition.z());

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

        case sMeshTypes::BeveledCube:
            return m_beveledCubeMesh;

        case sMeshTypes::Frustum:
            return m_frustumMesh;

        case sMeshTypes::Wedge:
            return m_wedgeMesh;

        case sMeshTypes::TriangularPrism:
            return m_triangularPrismMesh;

        case sMeshTypes::IcoSphere:
            return m_icoSphereMesh;

        case sMeshTypes::Rock:
            return m_rockMesh;

        case sMeshTypes::GrassBlade:
            return m_grassBladeMesh;

        case sMeshTypes::Capsule:
            return m_capsuleMesh;

        case sMeshTypes::Arch:
            return m_archMesh;

        case sMeshTypes::ExtrudedPolygon:
            return m_extrudedPolygonMesh;

        case sMeshTypes::Disc:
            return m_discMesh;

        case sMeshTypes::Arc:
            return m_arcMesh;

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
