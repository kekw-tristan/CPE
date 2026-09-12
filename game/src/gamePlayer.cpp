#include "gameInternal.h"

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayer(float _deltaTime)
{
    using namespace Engine;
    using namespace Engine::Platform;

    constexpr float c_moveSpeed         = 6.0f;
    constexpr float c_jumpVelocity      = 6.0f;
    constexpr float c_sprintMultiplier  = 1.5f;

    m_playerSpeedPotionTime = std::max(0.0f, m_playerSpeedPotionTime - _deltaTime);
    m_playerReflectionAuraTime = std::max(0.0f, m_playerReflectionAuraTime - _deltaTime);

    if (m_playerReflectionAuraTime > 0.0f)
    {
        m_playerReflectionAuraPhase = std::fmod(m_playerReflectionAuraPhase + _deltaTime * 2.4f, 6.28318530718f);

        GFX::sLight auraLight{};
        auraLight.type      = GFX::sLightType::Point;
        auraLight.color     = { 0.32f, 0.72f, 1.0f };
        auraLight.intensity = 3.0f;
        auraLight.position  = m_playerController.GetPosition() + Math::cVec3f(0.0f, 1.0f, 0.0f);
        auraLight.radius    = m_playerReflectionAuraRadius + 2.0f;

        if (m_playerReflectionAuraLight == GFX::c_invalidLightHandle)
            m_playerReflectionAuraLight = GFX::LightManager::CreateLight(auraLight);
        else
            GFX::LightManager::UpdateLight(m_playerReflectionAuraLight, auraLight);

        for (size_t shieldIndex = 0; shieldIndex < m_playerReflectionAuraShields.size(); ++shieldIndex)
        {
            GFX::sInstanceData*& rpShield = m_playerReflectionAuraShields[shieldIndex];
            if (rpShield == nullptr)
            {
                rpShield = m_pool.Create();
                rpShield->color = { 0.32f, 0.72f, 1.0f, 1.0f };
                rpShield->materialIndex = m_playerSphereMaterial;
                m_dynamicMeshInstances[m_beveledCubeMesh].push_back(rpShield);
                m_dynamicInstanceListDirty = true;
            }

            const float angle = m_playerReflectionAuraPhase + static_cast<float>(shieldIndex) * 1.57079632679f;
            GFX::sTransform shieldTransform{};
            shieldTransform.position = m_playerController.GetPosition() + Math::cVec3f(
                std::cos(angle) * m_playerReflectionAuraRadius,
                1.1f + std::sin(m_playerReflectionAuraPhase * 2.0f + static_cast<float>(shieldIndex)) * 0.12f,
                std::sin(angle) * m_playerReflectionAuraRadius);
            shieldTransform.rotation = { 0.0f, angle, 0.0f };
            shieldTransform.scale    = { 0.38f, 0.55f, 0.08f };
            rpShield->worldMatrix = CreateTransformMatrix(shieldTransform);
        }
    }
    else if (m_playerReflectionAuraLight != GFX::c_invalidLightHandle)
    {
        GFX::LightManager::DestroyLight(m_playerReflectionAuraLight);
        m_playerReflectionAuraLight = GFX::c_invalidLightHandle;
    }

    if (m_playerReflectionAuraTime <= 0.0f)
    {
        for (GFX::sInstanceData*& rpShield : m_playerReflectionAuraShields)
        {
            if (rpShield == nullptr)
                continue;

            std::erase(m_dynamicMeshInstances[m_beveledCubeMesh], rpShield);
            m_pool.Destroy(rpShield);
            rpShield = nullptr;
            m_dynamicInstanceListDirty = true;
        }
    }

    if (m_playerDashTime > 0.0f)
    {
        const float dashFrameTime = std::min(_deltaTime, m_playerDashTime);
        const float dashSpeed = _deltaTime > 0.0f ? m_playerDashSpeed * dashFrameTime / _deltaTime : 0.0f;

        m_playerController.Move(m_playerDashDirection, dashSpeed);
        m_playerDashTime = std::max(0.0f, m_playerDashTime - _deltaTime);
        m_playerYaw = std::atan2(m_playerDashDirection.x(), m_playerDashDirection.z());
        return;
    }

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

    const bool sprinting = IsKeyDown(340) || IsKeyDown(344);
    float moveSpeed = sprinting ? c_moveSpeed * c_sprintMultiplier : c_moveSpeed;

    if (m_playerSpeedPotionTime > 0.0f)
        moveSpeed *= c_speedPotionMultiplier;

    m_playerController.Move(movement, moveSpeed);

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
        const bool keyDown           = Engine::Platform::IsKeyDown(c_spellKeys[keyIndex]);
        spellKeysPressed[keyIndex]   = keyDown && !m_spellKeysWasDown[keyIndex];
        m_spellKeysWasDown[keyIndex] = keyDown;
    }

    m_playerAttackTime = std::max(0.0f, m_playerAttackTime - _deltaTime);

    const auto isSpellSlotHeld = [&](size_t _slot)
    {
        if (_slot == 0)
            return Engine::Platform::IsMouseButtonDown(c_leftMouseButton);

        if (_slot == 1)
            return Engine::Platform::IsMouseButtonDown(c_rightMouseButton);

        return _slot >= 2 && _slot < Gameplay::cRunState::c_numberOfSpellSlots
            && Engine::Platform::IsKeyDown(c_spellKeys[_slot - 2]);
    };

    if (m_playerChannelProjectileId != 0)
    {
        const Gameplay::cSpellInstance* pChannelSpell = m_runState.GetSpellInSlot(m_playerChannelSlot);
        if (pChannelSpell == nullptr)
        {
            m_playerChannelProjectileId = 0;
            m_playerChannelSlot = Gameplay::cRunState::c_numberOfSpellSlots;
            return;
        }

        const Gameplay::sSpellDefinition& channelDefinition = Gameplay::SpellManager::GetSpell(pChannelSpell->GetSpellId());
        const Gameplay::sSpellStats& channelStats = pChannelSpell->GetSpellStats();
        const float channelDuration = std::max(channelDefinition.channelDuration, 0.01f);

        m_playerChannelTime = std::min(channelDuration, m_playerChannelTime + _deltaTime);

        float cameraDirection[4];
        Engine::GFX::GetCamera().GetDirection(cameraDirection);

        Engine::Math::cVec3f direction(cameraDirection[0], cameraDirection[1], cameraDirection[2]);
        direction.normalize();

        if (direction.isZero())
            direction = { 0.0f, 0.0f, 1.0f };

        const float chargeFraction  = m_playerChannelTime / channelDuration;
        const float chargedRadius   = channelStats.projectileRadius * (0.5f + chargeFraction);
        
        const Engine::Math::cVec3f castPosition = m_playerController.GetPosition() + Engine::Math::cVec3f(0.0f, 1.25f, 0.0f);

        m_projectileManager.UpdatePlayerChannelCone(
            m_playerChannelProjectileId,
            castPosition + direction * 0.9f,
            direction,
            chargedRadius,
            1.25f + 1.75f * chargeFraction);

        if (!isSpellSlotHeld(m_playerChannelSlot) || m_playerChannelTime >= channelDuration)
        {
            const int projectileCount = std::max(1, channelStats.projectileCount);
            const float centerProjectile = 0.5f * static_cast<float>(projectileCount - 1);
            const float chargedDamage = channelStats.damage * (1.0f + 2.0f * chargeFraction);

            for (int projectileIndex = 0; projectileIndex < projectileCount; ++projectileIndex)
            {
                const float angle = (static_cast<float>(projectileIndex) - centerProjectile) * 0.12f;
                const float cosine = std::cos(angle);
                const float sine = std::sin(angle);
                const Engine::Math::cVec3f projectileDirection(
                    direction.x() * cosine + direction.z() * sine,
                    direction.y(),
                   -direction.x() * sine + direction.z() * cosine);

                if (projectileIndex == 0)
                {
                    m_projectileManager.ReleasePlayerChannelCone(
                        m_playerChannelProjectileId,
                        projectileDirection,
                        channelStats.projectileSpeed,
                        chargedDamage,
                        channelStats.duration);
                    continue;
                }

                Gameplay::sProjectileSpawnDesc projectile{};
                projectile.position     = castPosition + direction * 0.9f;
                projectile.direction    = projectileDirection;
                projectile.speed        = channelStats.projectileSpeed;
                projectile.damage       = chargedDamage;
                projectile.lifetime     = channelStats.duration;
                projectile.radius       = chargedRadius;
                projectile.visualScale  = 1.25f + 1.75f * chargeFraction;
                projectile.pierces      = channelStats.pierceCount;

                m_projectileManager.SpawnPlayerCone(projectile);
            }

            m_playerMana = std::max(0.0f, m_playerMana - channelDefinition.manaCost);
            
            m_runState.StartSpellCooldown(m_playerChannelSlot);
            
            m_playerAttackTime          = 0.4f;
            m_playerChannelTime         = 0.0f;
            m_playerChannelProjectileId = 0;
            m_playerChannelSlot         = Gameplay::cRunState::c_numberOfSpellSlots;
        }

        return;
    }

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
        && spellDefinition.castType != Gameplay::sSpellCastType::SporeProjectile
        && spellDefinition.castType != Gameplay::sSpellCastType::Dash
        && spellDefinition.castType != Gameplay::sSpellCastType::ChannelProjectile
        && spellDefinition.castType != Gameplay::sSpellCastType::ReflectionAura)
        return;

    if (m_playerMana < spellDefinition.manaCost)
        return;

    const Gameplay::sSpellStats& spellStats = pSpell->GetSpellStats();

    using Engine::Math::cVec3f;

    if (spellDefinition.castType == Gameplay::sSpellCastType::ReflectionAura)
    {
        m_playerReflectionAuraTime          = spellStats.duration;
        m_playerReflectionAuraRadius        = spellStats.projectileRadius;
        m_playerReflectionDamageMultiplier  = 1.0f + spellStats.damage * 0.01f;

        m_playerMana = std::max(0.0f, m_playerMana - spellDefinition.manaCost);

        m_runState.StartSpellCooldown(spellSlot);
        m_playerAttackTime = 0.4f;
        return;
    }

    if (spellDefinition.castType == Gameplay::sSpellCastType::ChannelProjectile)
    {
        float cameraDirection[4];
        Engine::GFX::GetCamera().GetDirection(cameraDirection);

        cVec3f direction(cameraDirection[0], cameraDirection[1], cameraDirection[2]);
        direction.normalize();

        if (direction.isZero())
            return;

        Gameplay::sProjectileSpawnDesc projectile{};
        projectile.position = m_playerController.GetPosition() + cVec3f(0.0f, 1.25f, 0.0f) + direction * 0.9f;
        projectile.direction = direction;
        projectile.radius = spellStats.projectileRadius * 0.5f;
        projectile.visualScale = 1.25f;
        projectile.lifetime = spellStats.duration;
        projectile.pierces = spellStats.pierceCount;

        m_playerChannelProjectileId = m_projectileManager.SpawnPlayerChannelCone(projectile);
        m_playerChannelSlot = spellSlot;
        m_playerChannelTime = 0.0f;
        return;
    }

    if (spellDefinition.castType == Gameplay::sSpellCastType::Dash)
    {
        float cameraDirection[4];
        Engine::GFX::GetCamera().GetDirection(cameraDirection);

        const cVec3f dashDirection(cameraDirection[0], 0.0f, cameraDirection[2]);
        if (dashDirection.isZero())
            return;

        m_playerDashDirection = dashDirection.normalized();
        m_playerDashSpeed     = spellStats.projectileSpeed;
        m_playerDashTime      = spellStats.duration;

        m_playerMana = std::max(0.0f, m_playerMana - spellDefinition.manaCost);
        m_runState.StartSpellCooldown(spellSlot);
        return;
    }

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
        const float angle   = (static_cast<float>(projectileIndex) - centerProjectile) * 0.12f;
        const float cosine  = std::cos(angle);
        const float sine    = std::sin(angle);

        Gameplay::sProjectileSpawnDesc projectile{};
        projectile.position  = castPosition;
        projectile.direction = cVec3f(
            direction.x() * cosine + direction.z() * sine,
            direction.y(),
           -direction.x() * sine + direction.z() * cosine).normalized();

        projectile.speed            = spellStats.projectileSpeed;
        projectile.damage           = spellStats.damage;
        projectile.lifetime         = spellStats.duration;
        projectile.radius           = spellStats.projectileRadius;
        projectile.isAreaOfEffect   = spellDefinition.castType == Gameplay::sSpellCastType::SporeProjectile;
        projectile.pierces          = spellStats.pierceCount;

        if (projectile.isAreaOfEffect)
        {
            projectile.areaRadius       = spellStats.projectileRadius;
            projectile.areaDuration     = spellStats.duration;
            projectile.areaGrowthTime   = 0.65f;
            
            const float targetDistance  = std::min(cVec3f::distance(castPosition, aimPosition), 18.0f);
            cVec3f      target          = castPosition + projectile.direction * targetDistance;
            float       groundHeight    = target.y();

            if (Engine::Physics::CollisionWorld::FindGroundHeight(target, target.y() + 3.0f, groundHeight))
                target = { target.x(), groundHeight + 0.2f, target.z() };

            Gameplay::AimMushroomThrow(projectile, target);
        }

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
    m_lootManager.Clear();

    m_playerMaxHealth   = c_playerBaseMaxHealth;
    m_playerHealth      = m_playerMaxHealth;
    m_playerMaxMana     = c_playerBaseMaxMana;
    m_playerMana        = m_playerMaxMana;
    m_playerSpeedPotionTime = 0.0f;
    m_playerDashTime    = 0.0f;
    m_playerDashSpeed   = 0.0f;
    m_playerChannelTime = 0.0f;
    m_playerReflectionAuraTime = 0.0f;
    m_playerReflectionAuraRadius = 0.0f;
    m_playerReflectionDamageMultiplier = 1.0f;
    m_playerReflectionAuraPhase = 0.0f;
    m_playerChannelSlot = Gameplay::cRunState::c_numberOfSpellSlots;
    m_playerChannelProjectileId = 0;
    m_playerDashDirection = {};

    if (!m_runState.GrantSpell(Gameplay::sSpellId::ArcaneOrb))
        return;

    const Gameplay::sSpellDefinition& starterSpell = Gameplay::SpellManager::GetSpell(Gameplay::sSpellId::ArcaneOrb);
    if (!m_inventory.AddItem(starterSpell.inventoryItem))
        return;

    m_inventory.AddItem({ Gameplay::sItemId::SpeedPotion, 10 });

    const auto& inventorySlots = m_inventory.GetInventorySlots();
    for (size_t inventorySlot = 0; inventorySlot < inventorySlots.size(); ++inventorySlot)
    {
        if (inventorySlots[inventorySlot].item != starterSpell.inventoryItem)
            continue;

        m_inventory.EquipSpell(inventorySlot, 0);
        break;
    }

    SyncSpellLoadoutFromInventory();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ApplyLevelUpRewards(uint32_t _levelUps)
{
    if (_levelUps == 0)
        return;

    const float healthBonus = c_levelUpHealthBonus * static_cast<float>(_levelUps);
    const float manaBonus   = c_levelUpManaBonus   * static_cast<float>(_levelUps);

    m_playerMaxHealth += healthBonus;
    m_playerHealth     = std::min(m_playerMaxHealth, m_playerHealth + healthBonus);
    m_playerMaxMana   += manaBonus;
    m_playerMana       = std::min(m_playerMaxMana, m_playerMana + manaBonus);
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

void cGame::UpdateUsableInput(bool _gameplayInputEnabled)
{
    constexpr std::array<int, Gameplay::cInventory::c_numberOfUsableSlots> c_usableKeys = { '1', '2', '3', '4' };

    const auto& usableSlots = m_inventory.GetUsableSlots();

    for (size_t slotIndex = 0; slotIndex < c_usableKeys.size(); ++slotIndex)
    {
        const bool keyDown      = Engine::Platform::IsKeyDown(c_usableKeys[slotIndex]);
        const bool keyPressed   = keyDown && !m_usableKeysWasDown[slotIndex];

        m_usableKeysWasDown[slotIndex] = keyDown;

        if (!_gameplayInputEnabled || !keyPressed)
            continue;

        const Gameplay::sItemStack& usableSlot = usableSlots[slotIndex];

        switch (usableSlot.item)
        {
            case Gameplay::sItemId::HealthPotion:
            {
                const float restoredHealth = std::min(m_playerMaxHealth, m_playerHealth + c_healthPotionRestore);
                if (restoredHealth > m_playerHealth && m_inventory.UseItem(slotIndex))
                    m_playerHealth = restoredHealth;
                break;
            }

            case Gameplay::sItemId::ManaPotion:
            {
                const float restoredMana = std::min(m_playerMaxMana, m_playerMana + c_manaPotionRestore);
                if (restoredMana > m_playerMana && m_inventory.UseItem(slotIndex))
                    m_playerMana = restoredMana;
                break;
            }

            case Gameplay::sItemId::SpeedPotion:
                if (m_inventory.UseItem(slotIndex))
                    m_playerSpeedPotionTime = c_speedPotionDuration;
                break;
        }
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

    const float mouseDeltaX     = m_mouseReleased ? 0.0f : Platform::GetMouseDeltaX();
    const float mouseDeltaY     = m_mouseReleased ? 0.0f : Platform::GetMouseDeltaY();
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

    const Math::cVec3f cameraRight      = cameraDirection.cross(Math::cVec3f(0.0f, 1.0f, 0.0f)).normalized();
    const Math::cVec3f targetPosition   = m_playerController.GetPosition() + Math::cVec3f(0.0f, c_targetHeight, 0.0f) + cameraRight * c_shoulderOffset;

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
