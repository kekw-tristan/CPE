#include "gameInternal.h"

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
    , m_dynamicInstances()
    , m_playerModel()
    , m_playerRenderParts()
    , m_playerController()
    , m_playerYaw(0.f)
    , m_cameraPitch(-10.f)
    , m_dynamicMeshInstances()
    , m_inventory()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    InitMeshes();

    InitNightSky();

    World::WorldGenerator::Generate(1337);
    m_playerController.SetPosition({ 0.0f, World::GetTerrainSurfaceHeight(0.0f, -24.0f) + 0.1f, -24.0f });

#if defined(GAME_DEBUG)
    if (m_mushroomPreview)
    {
        for (const auto& dungeon : World::WorldGenerator::GetLayout().dungeons)
        {
            if (dungeon.bossId != World::sBossId::ForestSporecap)
                continue;

            m_playerController.SetPosition(dungeon.center + Math::cVec3f(0.0f,
                World::GetBossArenaHeight(dungeon.bossId) + 0.1f, -99.0f * World::c_mushroomDungeonScale));
            const auto eye = dungeon.center + Math::cVec3f(0.0f, 246.0f, -119.0f) * World::c_mushroomDungeonScale;
            const auto target = dungeon.center + Math::cVec3f(0.0f, 262.0f, 0.0f) * World::c_mushroomDungeonScale;
            GFX::GetCamera().LookAt(eye.x(), eye.y(), eye.z(), target.x(), target.y(), target.z());
            World::WorldGenerator::Update(m_playerController.GetPosition(),
                (2 * World::c_chunkLoadRadius + 1) * (2 * World::c_chunkLoadRadius + 1));
            break;
        }
    }
#endif

    if (LoadPlayerModel())
        BuildPlayerRenderInstances();

    m_enemyModelsLoaded = LoadEnemyModels();
    RefreshWorldRenderInstances();

    UpdateEnemyRenderInstances(0.0f);

    RebuildDynamicInstanceList();

    Platform::SetMouseCaptured(true);
#if defined(GAME_DEBUG)
    if (m_mushroomPreview)
        Platform::SetMouseCaptured(false);
#endif

    BeginRun();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
#if defined(GAME_DEBUG)
    // Stable inspection view: gameplay remains untouched in a normal launch.
    if (m_mushroomPreview)
    {
        UpdatePlayerRenderInstances();
        UpdateDungeonAtmosphere();
        UpdateProjectileEffects(_deltaTime);
        if (m_dynamicInstanceListDirty)
            RebuildDynamicInstanceList();
        return;
    }
#endif

    UpdateInventoryInput();

    const bool augmentSelectionPending = m_runState.HasPendingAugmentSelection();
    UpdateUsableInput(!m_inventoryOpen && !augmentSelectionPending);

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
        UpdatePlayer(_deltaTime);

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
        m_projectileManager.Update(_deltaTime, enemyContext.playerPosition, m_enemyManager,
            m_playerReflectionAuraTime, m_playerReflectionAuraRadius, m_playerReflectionDamageMultiplier);

        for (const Gameplay::sEnemyDeathEvent& deathEvent : m_enemyManager.GetDeathEvents())
        {
            const uint32_t experience = deathEvent.tier == World::sEnemyTier::Unique ? c_bossExperience
                : deathEvent.tier == World::sEnemyTier::Yellow ? c_yellowEnemyExperience
                : deathEvent.tier == World::sEnemyTier::Blue ? c_blueEnemyExperience
                : c_regularEnemyExperience;
            ApplyLevelUpRewards(m_runState.GrantExperience(experience));
            m_lootManager.DropEnemyLoot(deathEvent.position, deathEvent.isBoss, deathEvent.bossId);
        }

        m_enemyManager.ClearDeathEvents();

        m_lootManager.CollectNearby(m_playerController.GetPosition(), m_inventory);
        bool hasCollectedSpell = false;
        for (const Gameplay::sItemStack& item : m_lootManager.GetCollectedItems())
        {
            const Gameplay::sSpellId::Enum spellId = Gameplay::SpellManager::GetSpellId(item.item);
            if (spellId != Gameplay::sSpellId::Undefined)
            {
                m_runState.GrantSpell(spellId);
                hasCollectedSpell = true;
            }
        }

        if (hasCollectedSpell)
            SyncSpellLoadoutFromInventory();

        m_lootManager.ClearCollectedItems();

        const float receivedDamage = m_enemyManager.ConsumePlayerDamage() + m_projectileManager.ConsumePlayerDamage();
        if (receivedDamage > 0.0f)
        {
            const float absorbedDamage = std::min(receivedDamage, static_cast<float>(m_inventory.GetArmor()));
            m_playerHealth = std::max(0.0f, m_playerHealth - receivedDamage + absorbedDamage);
            std::cout << "Player health: " << m_playerHealth << '\n';
        }

        if (m_runState.HasPendingAugmentSelection())
            Engine::Platform::SetMouseCaptured(false);
    }

    UpdatePlayerRenderInstances();
    UpdateEnemyRenderInstances(_deltaTime);
    SyncProjectileRenderInstances();
    UpdateDungeonAtmosphere();
    UpdateProjectileEffects(augmentSelectionPending ? 0.0f : _deltaTime);
    SyncLootRenderInstances();

    if (m_dynamicInstanceListDirty)
        RebuildDynamicInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateDungeonAtmosphere()
{
    GFX::sLocalAtmosphereSettings atmosphere{};
    bool emitSpores = false;

    for (const auto& dungeon : World::WorldGenerator::GetLayout().dungeons)
    {
        if (dungeon.bossId != World::sBossId::ForestSporecap)
            continue;

        const auto& center = dungeon.center;
        constexpr float c_scale = World::c_mushroomDungeonScale;
        atmosphere.boundsMinBlend = { center.x() - 246.0f * c_scale, center.y() - 8.0f * c_scale,
            center.z() - 246.0f * c_scale, 4.0f };
        atmosphere.boundsMaxAmbient = { center.x() + 246.0f * c_scale, center.y() + 328.0f * c_scale,
            center.z() + 246.0f * c_scale, 0.72f };
        atmosphere.fogColorDensity = { 0.025f, 0.045f, 0.14f, 0.0035f };
        atmosphere.fogHeightStart = { center.y() + 80.0f * c_scale, 12.0f, 0.035f, 0.009f };
        atmosphere.shaftTopRadius = { center.x(), center.y() + 324.0f * c_scale, center.z(), 19.0f * c_scale };
        atmosphere.shaftBottomRadius = { center.x(), center.y() + 234.1f * c_scale, center.z(), 33.0f * c_scale };
        atmosphere.shaftColorDensity = { 0.14f, 0.30f, 0.65f, 0.012f };

        const auto offset = m_playerController.GetPosition() - center;
        emitSpores = offset.x() * offset.x() + offset.z() * offset.z() < 210.0f * 210.0f * c_scale * c_scale
            && offset.y() > 195.0f * c_scale && offset.y() < 327.0f * c_scale;
        if (emitSpores && !m_particleSystem.IsAlive(m_dungeonSporeEmitter))
        {
            GFX::sParticleDefinition spores{};
            spores.spawnRate = 9.0f;
            spores.lifetime = 12.0f;
            spores.startSize = 0.045f;
            spores.endSize = 0.018f;
            spores.speed = 0.14f;
            spores.spread = 0.10f;
            spores.startColor = { 0.40f, 0.85f, 1.8f, 0.65f };
            spores.endColor = { 0.25f, 0.55f, 1.2f, 0.0f };
            m_dungeonSporeEmitter = m_particleSystem.CreateEmitter(spores, center + Math::cVec3f(0.0f, 260.0f * c_scale, 0.0f));
            std::array<GFX::sParticleSurface, 4> surfaces{};
            for (size_t i = 0; i < surfaces.size(); ++i)
            {
                surfaces[i].position = center + Math::cVec3f(0.0f, (242.0f + static_cast<float>(i) * 21.0f) * c_scale, 0.0f);
                surfaces[i].radius = 30.0f * c_scale;
            }
            m_particleSystem.SetSurfaces(m_dungeonSporeEmitter, surfaces);
        }
        break;
    }

    if (!emitSpores)
    {
        m_particleSystem.StopEmitter(m_dungeonSporeEmitter, true);
        m_dungeonSporeEmitter = {};
    }
    GFX::SetLocalAtmosphere(atmosphere);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnPrepareRender()
{
    Engine::GFX::UpdateInstanceBuffer(m_staticInstances, m_staticInstanceRevision, m_dynamicInstances);

    PrepareEnemyHealthBars(Engine::GFX::GetCamera());
    Engine::GFX::UpdateHealthBars(m_healthBars);

    float position[4];
    float direction[4];
    Engine::GFX::GetCamera().GetPosition(position);
    Engine::GFX::GetCamera().GetDirection(direction);
    Engine::GFX::UpdateParticles(m_particleSystem.PrepareRender({ position[0], position[1], position[2] }, { direction[0], direction[1], direction[2] }));
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnDraw()
{
    for (auto& [coordinate, chunk] : m_worldRenderInstances)
    {
        chunk.visible = GFX::IsBoundsVisible(chunk.bounds);
    }

    GFX::MeshHandle mesh = nullptr;
    uint32_t firstInstance = 0;
    uint32_t instanceCount = 0;

    for (const sWorldDrawBatch& batch : m_worldDrawBatches)
    {
        if (!batch.pChunk->visible)
            continue;

        if (instanceCount != 0 && (mesh != batch.mesh || firstInstance + instanceCount != batch.firstInstance))
        {
            GFX::DrawMeshIntances(mesh, instanceCount, firstInstance);
            instanceCount = 0;
        }

        if (instanceCount == 0)
        {
            mesh = batch.mesh;
            firstInstance = batch.firstInstance;
        }
        instanceCount += batch.instanceCount;
    }

    if (instanceCount != 0)
        GFX::DrawMeshIntances(mesh, instanceCount, firstInstance);

    firstInstance = static_cast<uint32_t>(m_staticInstances.size());

    for (auto& [mesh, instances] : m_dynamicMeshInstances)
    {
        if (instances.empty())
            continue;

        Engine::GFX::DrawMeshIntances(mesh, static_cast<uint32_t>(instances.size()), firstInstance);

        firstInstance += static_cast<uint32_t>(instances.size());
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnDrawUI()
{
#if defined(GAME_DEBUG)
    if (m_mushroomPreview)
        return;
#endif

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

    float cameraDirection[4]{};
    Engine::GFX::GetCamera().GetDirection(cameraDirection);
    hudState.cameraYaw = std::atan2(cameraDirection[0], cameraDirection[2]);

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

        if (m_playerChannelProjectileId != 0 && slotIndex == m_playerChannelSlot)
        {
            hudState.channelSlot = slotIndex;
            const float duration = Gameplay::SpellManager::GetSpell(pSpell->GetSpellId()).channelDuration;
            hudState.channelFraction = std::clamp(m_playerChannelTime / std::max(duration, 0.01f), 0.0f, 1.0f);
            hudState.spellActiveFractions[slotIndex] = hudState.channelFraction;
        }
        else if (pSpell->GetSpellId() == Gameplay::sSpellId::StoneShard)
        {
            hudState.spellActiveFractions[slotIndex] = m_playerReflectionAuraTime / std::max(m_playerReflectionAuraDuration, 0.01f);
        }
        else if (pSpell->GetSpellId() == Gameplay::sSpellId::Dash)
        {
            hudState.spellActiveFractions[slotIndex] = m_playerDashTime / std::max(m_playerDashDuration, 0.01f);
        }
    }

    // Navigation uses immutable layout data even before an arena's chunk is loaded.
    for (const auto& definition : World::WorldGenerator::GetLayout().dungeons)
    {
        auto& dungeon = hudState.dungeons[static_cast<size_t>(definition.bossId)];
        const auto arenaPosition = definition.center + Math::cVec3f(0.0f, World::GetBossArenaHeight(definition.bossId), 0.0f);
        const auto offset = arenaPosition - m_playerController.GetPosition();

        dungeon.offsetX  = offset.x();
        dungeon.offsetZ  = offset.z();
        dungeon.distance = std::sqrt(offset.x() * offset.x() + offset.z() * offset.z());
        dungeon.inArena = World::IsInsideBossArena(definition.bossId, m_playerController.GetPosition(), arenaPosition);

        const auto localPosition = m_playerController.GetPosition() - definition.center;
        if (definition.bossId == World::sBossId::ForestSporecap
            && std::abs(localPosition.x()) < 90.0f && localPosition.z() > -216.0f && localPosition.z() < 90.0f)
        {
            if (dungeon.inArena)
                dungeon.objective = "Hutkrone: Besiege den Sporenkoenig.";
            else if (localPosition.y() >= 74.0f)
                dungeon.objective = "Aussenaufstieg: Folge der Treppe auf den Pilzhut.";
            else if (localPosition.y() >= 71.0f)
                dungeon.objective = "Kronensaal: Der Westausgang fuehrt zum Pilzhut.";
            else if (localPosition.y() >= 47.0f)
                dungeon.objective = "Alchemie: Folge der Wendeltreppe nach oben.";
            else if (localPosition.y() >= 23.0f)
                dungeon.objective = "Pilzgaerten: Folge der Wendeltreppe nach oben.";
            else
                dungeon.objective = "Empfangshalle: Folge der Treppe zur Hutkrone.";
        }
    }

    // quest
    for (const auto& handle : m_bossHandles)
    {
        const auto* pEnemy = m_enemyManager.TryGetEnemy(handle);
        if (pEnemy == nullptr || !pEnemy->isBoss)
            continue;

        auto& dungeon = hudState.dungeons[static_cast<size_t>(pEnemy->bossId)];

        dungeon.defeated       = pEnemy->state == Gameplay::eEnemyState::Dead;
        dungeon.healthFraction = pEnemy->health / pEnemy->definition.maxHealth;
    }

    const auto& inventorySlots = m_inventory.GetInventorySlots();

    // inventory
    for (size_t i = 0; i < inventorySlots.size(); ++i)
    {
        hudState.inventory.inventorySlots[i].item   = inventorySlots[i].item;
        hudState.inventory.inventorySlots[i].amount = inventorySlots[i].amount;
        hudState.inventory.inventorySlots[i].rarity = inventorySlots[i].rarity;
        hudState.inventory.inventorySlots[i].armor  = inventorySlots[i].armor;
    }

    const auto& usableSlots = m_inventory.GetUsableSlots();

    for (size_t i = 0; i < usableSlots.size(); ++i)
    {
        hudState.inventory.usableSlots[i].item   = usableSlots[i].item;
        hudState.inventory.usableSlots[i].amount = usableSlots[i].amount;
        hudState.inventory.usableSlots[i].rarity = usableSlots[i].rarity;
        hudState.inventory.usableSlots[i].armor  = usableSlots[i].armor;
    }

    const auto& spellSlots = m_inventory.GetSpellSlots();

    for (size_t i = 0; i < spellSlots.size(); ++i)
    {
        hudState.inventory.spellSlots[i].item   = spellSlots[i].item;
        hudState.inventory.spellSlots[i].amount = spellSlots[i].amount;
        hudState.inventory.spellSlots[i].rarity = spellSlots[i].rarity;
        hudState.inventory.spellSlots[i].armor  = spellSlots[i].armor;
    }

    const auto& armorSlots = m_inventory.GetArmorSlots();

    for (size_t i = 0; i < armorSlots.size(); ++i)
    {
        hudState.inventory.armorSlots[i].item   = armorSlots[i].item;
        hudState.inventory.armorSlots[i].amount = armorSlots[i].amount;
        hudState.inventory.armorSlots[i].rarity = armorSlots[i].rarity;
        hudState.inventory.armorSlots[i].armor  = armorSlots[i].armor;
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

            case UI::eInventoryAction::MoveUsable:
                m_inventory.MoveUsable(sourceInventorySlot, destinationInventorySlot);
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
    {
        GFX::ShapeModelLights::Destroy(instances.lightHandles);
        for (GFX::ReflectionProbeHandle probeHandle : instances.reflectionProbeHandles)
            GFX::ReflectionProbeManager::RemoveProbe(probeHandle);
    }

    m_worldRenderInstances.clear();
    m_bakedWorldModels.clear();

    m_enemyManager.Clear();
    m_projectileManager.Clear();
    m_particleSystem.Clear();
    GFX::LightManager::DestroyLight(m_playerReflectionAuraLight);
    m_playerReflectionAuraLight = GFX::c_invalidLightHandle;

    for (sEnemyVisual& visual : m_enemyVisuals)
    {
        GFX::ShapeModelLights::Destroy(visual.lightHandles);
        GFX::LightManager::DestroyLight(visual.auraLight);
    }

    m_enemyVisuals.clear();
    m_healthBars.clear();

    for (const sProjectileVisual& visual : m_projectileVisuals)
        GFX::LightManager::DestroyLight(visual.light);

    m_projectileVisuals.clear();
    GFX::ShapeModelLights::Destroy(m_playerLightHandles);
    ClearRenderInstances();
}

// -------------------------------------------------------------------------------------------------------------------------
