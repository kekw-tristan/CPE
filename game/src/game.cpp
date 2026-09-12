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

    if (LoadPlayerModel())
        BuildPlayerRenderInstances();

    m_enemyModelsLoaded = LoadEnemyModels();
    RefreshWorldRenderInstances();

    UpdateEnemyRenderInstances(0.0f);

    RebuildDynamicInstanceList();

    Platform::SetMouseCaptured(true);

    BeginRun();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
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
            const uint32_t experience = deathEvent.isBoss ? c_bossExperience : c_regularEnemyExperience;
            ApplyLevelUpRewards(m_runState.GrantExperience(experience));
            m_lootManager.DropEnemyLoot(deathEvent.position, deathEvent.isBoss, deathEvent.bossId);
        }

        m_enemyManager.ClearDeathEvents();

        m_lootManager.CollectNearby(m_playerController.GetPosition(), m_inventory);
        for (const Gameplay::sItemStack& item : m_lootManager.GetCollectedItems())
        {
            const Gameplay::sSpellId::Enum spellId = Gameplay::SpellManager::GetSpellId(item.item);
            if (spellId != Gameplay::sSpellId::Undefined)
                m_runState.GrantSpell(spellId);
        }
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
    UpdateProjectileEffects(augmentSelectionPending ? 0.0f : _deltaTime);
    SyncLootRenderInstances();

    if (m_dynamicInstanceListDirty)
        RebuildDynamicInstanceList();
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
