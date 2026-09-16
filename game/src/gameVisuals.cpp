#include "gameInternal.h"

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
    playerTransform.scale    = { 1.0f, 1.0f, 1.0f };

    const float dashWeight = std::clamp(m_playerDashVisualTime / 0.18f, 0.0f, 1.0f)
        * std::clamp((m_playerDashDuration + 0.18f - m_playerDashVisualTime) / 0.06f, 0.0f, 1.0f);

    playerTransform.rotation = { 0.30f * dashWeight - 0.045f * m_playerChannelPoseWeight, m_playerYaw, 0.0f };
    playerTransform.scale = { 1.0f + 0.04f * dashWeight, 1.0f - 0.10f * dashWeight, 1.0f };

    const cMatrix4x4f playerMatrix = CreateTransformMatrix(playerTransform);

    for (size_t streakIndex = 0; streakIndex < m_playerDashStreaks.size(); ++streakIndex)
    {
        sInstanceData*& rpStreak = m_playerDashStreaks[streakIndex];
        if (dashWeight <= 0.0f)
        {
            if (rpStreak != nullptr)
            {
                std::erase(m_dynamicMeshInstances[m_crystalMesh], rpStreak);
                m_pool.Destroy(rpStreak);
                rpStreak = nullptr;
                m_dynamicInstanceListDirty = true;
            }
            continue;
        }

        if (rpStreak == nullptr)
        {
            rpStreak = m_pool.Create();
            rpStreak->materialIndex = m_sapSpellMaterial;
            rpStreak->color = { 0.66f, 0.92f, 0.63f, 1.0f };
            m_dynamicMeshInstances[m_crystalMesh].push_back(rpStreak);
            m_dynamicInstanceListDirty = true;
        }

        const float side = streakIndex % 2 == 0 ? -1.0f : 1.0f;
        const float layer = static_cast<float>(streakIndex / 2);
        const cVec3f right(m_playerDashDirection.z(), 0.0f, -m_playerDashDirection.x());
        sTransform streak{};
        streak.position = m_playerController.GetPosition()
            + right * (side * (0.42f + layer * 0.13f))
            + cVec3f(0.0f, 0.55f + layer * 0.38f, 0.0f)
            - m_playerDashDirection * (0.48f + layer * 0.24f);
        streak.rotation = { 1.57079632679f, std::atan2(m_playerDashDirection.x(), m_playerDashDirection.z()), 0.0f };
        streak.scale = { 0.055f * dashWeight, (1.65f - layer * 0.3f) * dashWeight, 0.035f * dashWeight };
        rpStreak->worldMatrix = CreateTransformMatrix(streak);
    }

    const float attackAge = 0.4f - m_playerAttackTime;
    float attackWeight = attackAge < 0.06f ? attackAge / 0.06f : m_playerAttackTime / 0.34f;

    attackWeight = std::clamp(attackWeight, 0.0f, 1.0f);
    attackWeight = attackWeight * attackWeight * (3.0f - 2.0f * attackWeight);
    attackWeight = std::max(attackWeight, m_playerChannelPoseWeight * (0.72f + 0.04f * std::sin(m_playerChannelTime * 8.0f)));
    attackWeight *= 1.0f - 0.75f * dashWeight;

    for (size_t partIndex = 0; partIndex < m_playerRenderParts.size(); ++partIndex)
    {
        sPlayerRenderPart&  renderPart    = m_playerRenderParts[partIndex];
        const sTransform    partTransform = m_playerAttackModel.shapes.empty() ? renderPart.transform : InterpolateTransform(renderPart.transform, m_playerAttackModel.shapes[partIndex].transform, attackWeight);
        const cMatrix4x4f   partMatrix    = CreateTransformMatrix(partTransform);

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
            : pEnemy->type == World::sEnemyType::ForestThornshooter ? 2.8f
            : pEnemy->type == World::sEnemyType::ForestRootcharger ? 2.4f
            : pEnemy->type == World::sEnemyType::ForestBarkguard ? 3.0f
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
        bar.heightFill[0]    = c_healthBarHeight;
        bar.heightFill[1]    = std::clamp(pEnemy->health / maxHealth, 0.0f, 1.0f);
        bar.heightFill[2]    = static_cast<float>(pEnemy->tier);

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
        {
            m_particleSystem.StopEmitter(visual.spellEmitter);
            continue;
        }

        const bool hasReflectionAura = pEnemy->state != Gameplay::eEnemyState::Dead && pEnemy->projectileReflectionAuraTime > 0.0f;
        if (hasReflectionAura)
        {
            visual.auraAge += _deltaTime;
            const float growth = std::clamp(visual.auraAge / 0.22f, 0.0f, 1.0f);
            const float wardWeight = growth * std::clamp(pEnemy->projectileReflectionAuraTime / 0.35f, 0.0f, 1.0f);

            sLight auraLight{};
            auraLight.type      = sLightType::Point;
            auraLight.color     = { 0.82f, 0.48f, 0.16f };
            auraLight.intensity = 1.8f * wardWeight;
            auraLight.position  = pEnemy->position + cVec3f(0.0f, 1.4f * pEnemy->scale, 0.0f);
            auraLight.radius    = 4.0f * pEnemy->scale;

            if (visual.auraLight == c_invalidLightHandle)
                visual.auraLight = LightManager::CreateLight(auraLight);
            else
                LightManager::UpdateLight(visual.auraLight, auraLight);

            visual.auraShieldPhase = std::fmod(visual.auraShieldPhase + _deltaTime * 1.2f, 6.28318530718f);
            for (size_t shieldIndex = 0; shieldIndex < visual.auraShields.size(); ++shieldIndex)
            {
                const bool isInlay = shieldIndex >= 4;
                const float shieldPhase = static_cast<float>(shieldIndex % 4);
                sInstanceData*& rpShield = visual.auraShields[shieldIndex];
                if (rpShield == nullptr)
                {
                    rpShield = m_pool.Create();
                    rpShield->color = isInlay ? std::array<float, 4>{ 0.82f, 0.54f, 0.20f, 1.0f }
                        : std::array<float, 4>{ 0.30f, 0.22f, 0.09f, 1.0f };
                    rpShield->materialIndex = isInlay ? m_hostileSpellMaterial : m_barkSpellMaterial;
                    m_dynamicMeshInstances[m_beveledCubeMesh].push_back(rpShield);
                    m_dynamicInstanceListDirty = true;
                }

                const float angle = visual.auraShieldPhase + shieldPhase * 1.57079632679f;
                const float radius = (1.8f * (0.65f + 0.35f * growth) + (isInlay ? 0.09f : 0.0f)) * pEnemy->scale;

                sTransform shieldTransform{};
                shieldTransform.position = pEnemy->position + cVec3f(
                    std::cos(angle) * radius,
                    (1.4f + std::sin(visual.auraShieldPhase * 2.0f + shieldPhase) * 0.12f) * pEnemy->scale,
                    std::sin(angle) * radius);

                shieldTransform.rotation = { 0.12f * std::sin(angle), 1.57079632679f - angle, 0.12f * std::cos(angle) };
                shieldTransform.scale = cVec3f(0.58f, 1.05f, 0.16f) * (wardWeight * pEnemy->scale);
                if (isInlay)
                {
                    shieldTransform.position += cVec3f(0.0f, (shieldIndex >= 8 ? 0.2f : -0.2f) * pEnemy->scale, 0.0f);
                    shieldTransform.rotation += cVec3f(0.0f, 0.0f, shieldIndex >= 8 ? 0.5f : -0.5f);
                    shieldTransform.scale = cVec3f(0.055f, 0.48f, 0.035f) * (wardWeight * pEnemy->scale);
                }
                rpShield->worldMatrix = CreateTransformMatrix(shieldTransform);
            }
        }
        else if (visual.auraLight != c_invalidLightHandle)
        {
            LightManager::DestroyLight(visual.auraLight);
            visual.auraLight = c_invalidLightHandle;
        }

        if (!hasReflectionAura)
        {
            visual.auraAge = 0.0f;
            for (sInstanceData*& rpShield : visual.auraShields)
            {
                if (rpShield == nullptr)
                    continue;

                std::erase(m_dynamicMeshInstances[m_beveledCubeMesh], rpShield);
                m_pool.Destroy(rpShield);
                rpShield = nullptr;
                m_dynamicInstanceListDirty = true;
            }
        }

        const bool isAttacking = pEnemy->state == Gameplay::eEnemyState::AttackWindup || pEnemy->state == Gameplay::eEnemyState::AttackRecovery
            || pEnemy->state == Gameplay::eEnemyState::Dash;

        const bool isDashing = pEnemy->state == Gameplay::eEnemyState::Dash;
        const bool isChargingThorn = pEnemy->state == Gameplay::eEnemyState::AttackWindup
            && pEnemy->definition.attackType == Gameplay::eEnemyAttackType::ConeProjectile;

        sParticleDefinition spellMotes{};
        spellMotes.spawnRate    = isDashing ? 110.0f : 42.0f;
        spellMotes.lifetime     = 0.32f;
        spellMotes.startSize    = (isDashing ? 0.12f : 0.065f) * pEnemy->scale;
        spellMotes.endSize      = 0.015f;
        spellMotes.speed        = 0.15f;
        spellMotes.spread       = isDashing ? 0.8f : 0.12f;
        spellMotes.acceleration = { 0.0f, -1.5f, 0.0f };
        spellMotes.startColor   = { 0.88f, 0.48f, 0.15f, 0.7f };
        spellMotes.endColor     = { 0.42f, 0.24f, 0.08f, 0.0f };

        const cVec3f castPosition = pEnemy->position + cVec3f(0.0f, (isDashing ? 0.65f : 1.0f) * pEnemy->scale, 0.0f);
        if (visual.emittingDash != isDashing)
        {
            if (visual.emittingDash && m_particleSystem.IsAlive(visual.spellEmitter) && pEnemy->state != Gameplay::eEnemyState::Dead)
                m_particleSystem.Burst(spellMotes, castPosition, 12);
            m_particleSystem.StopEmitter(visual.spellEmitter);
        }

        visual.emittingDash = isDashing;
        if (isDashing || isChargingThorn)
        {
            if (!m_particleSystem.IsAlive(visual.spellEmitter))
            {
                visual.spellEmitter = m_particleSystem.CreateEmitter(spellMotes, castPosition);
                if (isDashing)
                    m_particleSystem.Burst(spellMotes, castPosition, 18);
            }
            const float phase = pEnemy->stateTime * 10.0f;
            const cVec3f chargeOrbit = cVec3f(std::cos(phase) * 0.3f, std::sin(phase) * 0.3f, 0.7f) * pEnemy->scale;
            const cVec3f offset = isChargingThorn ? cMatrix4x4f::rotationY(pEnemy->rotation).transformDirection(chargeOrbit) : cVec3f{};
            m_particleSystem.SetPosition(visual.spellEmitter, castPosition + offset);
        }

        else if (m_particleSystem.IsAlive(visual.spellEmitter))
        {
            m_particleSystem.StopEmitter(visual.spellEmitter);
            if (pEnemy->state != Gameplay::eEnemyState::Dead)
                m_particleSystem.Burst(spellMotes, castPosition, 12);
        }

        const bool isThornwolf      = pEnemy->type == World::sEnemyType::ForestThornwolf;
        const bool hasWalkAnimation = isThornwolf || pEnemy->type == World::sEnemyType::ForestSporecap
            || pEnemy->type == World::sEnemyType::ForestThornshooter
            || pEnemy->type == World::sEnemyType::ForestRootcharger
            || pEnemy->type == World::sEnemyType::ForestBarkguard;
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

        if (pEnemy->definition.attackType == Gameplay::eEnemyAttackType::Dash && isAttacking)
        {
            const float dashWeight = isDashing ? std::clamp(pEnemy->stateTime / 0.06f, 0.0f, 1.0f)
                : pEnemy->state == Gameplay::eEnemyState::AttackRecovery ? pEnemy->attackPoseWeight : 0.0f;
            enemyTransform.rotation = { 0.20f * dashWeight, pEnemy->rotation, 0.0f };
            enemyTransform.scale = cVec3f(1.0f + 0.04f * dashWeight, 1.0f - 0.10f * dashWeight, 1.0f) * pEnemy->scale;
        }

        const cMatrix4x4f enemyMatrix = CreateTransformMatrix(enemyTransform);

        const sShapeModelDesc& attackModel = pEnemy->type == World::sEnemyType::ForestCrawler ? m_enemy03AttackModel
            : pEnemy->type == World::sEnemyType::ForestThornwolf ? m_thornwolfAttackModel
            : pEnemy->type == World::sEnemyType::ForestSporecap ? m_sporecapAttackModel
            : pEnemy->type == World::sEnemyType::ForestThornshooter ? m_thornshooterAttackModel
            : pEnemy->type == World::sEnemyType::ForestRootcharger ? m_rootchargerAttackModel
            : pEnemy->type == World::sEnemyType::ForestBarkguard ? m_barkguardAttackModel
            : m_enemy04AttackModel;

        for (size_t partIndex = 0; partIndex < visual.renderParts.size(); ++partIndex)
        {
            sEnemyRenderPart& renderPart    = visual.renderParts[partIndex];
            sTransform partTransform = attackModel.shapes.empty() ? renderPart.transform : InterpolateTransform(renderPart.transform, attackModel.shapes[partIndex].transform, pEnemy->attackPoseWeight);

            const float walkWeight = visual.walkWeight * (1.0f - pEnemy->attackPoseWeight);
            if (hasWalkAnimation && walkWeight > 0.0f)
            {
                // Forest models keep their feet below y=0.5; use the rest pose to identify legs.
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

        const bool isReflected = projectile.type == Gameplay::eProjectileType::EnemyReflected
            || projectile.type == Gameplay::eProjectileType::PlayerReflected;

        const bool isThorn = projectile.type == Gameplay::eProjectileType::PlayerCone
            || projectile.type == Gameplay::eProjectileType::EnemyCone || (isReflected && projectile.reflectedCone);

        const bool isArcane  = projectile.type == Gameplay::eProjectileType::PlayerSphere || (isReflected && !projectile.reflectedCone);

        const bool isHostile = projectile.type == Gameplay::eProjectileType::EnemyCone
            || projectile.type == Gameplay::eProjectileType::EnemySpore || projectile.type == Gameplay::eProjectileType::EnemyReflected;

        auto visual = std::find_if(m_projectileVisuals.begin(), m_projectileVisuals.end(), [&projectile](const sProjectileVisual& _rVisual)
        {
            return _rVisual.id == projectile.id;
        });

        if (visual == m_projectileVisuals.end())
        {
            sInstanceData* pInstance = m_pool.Create();

            const bool isEnemyReflected  = projectile.type == Gameplay::eProjectileType::EnemyReflected;
            const bool isPlayerReflected = projectile.type == Gameplay::eProjectileType::PlayerReflected;
            const bool isPlayerSpell     = projectile.type == Gameplay::eProjectileType::PlayerSphere
                || projectile.type == Gameplay::eProjectileType::PlayerCone
                || projectile.type == Gameplay::eProjectileType::PlayerSpore
                || isPlayerReflected;

            const bool isPlayerSpore = projectile.type == Gameplay::eProjectileType::PlayerSpore;
            const bool isSpore       = isPlayerSpore || projectile.type == Gameplay::eProjectileType::EnemySpore;

            pInstance->color = isEnemyReflected
                ? std::array<float, 4>{ 1.0f, 0.42f, 0.08f, 1.0f }
                : isPlayerReflected
                ? std::array<float, 4>{ 0.32f, 0.72f, 1.0f, 1.0f }
                : isPlayerSpore
                ? std::array<float, 4>{ 0.27f, 0.43f, 0.12f, 1.0f }
                : isPlayerSpell
                    ? projectile.type == Gameplay::eProjectileType::PlayerCone
                        ? std::array<float, 4>{ 0.24f, 0.53f, 0.20f, 1.0f }
                        : std::array<float, 4>{ 0.46f, 0.82f, 0.98f, 1.0f }
                    : projectile.type == Gameplay::eProjectileType::EnemyShockwave
                        ? std::array<float, 4>{ 0.85f, 0.46f, 0.12f, 1.0f }
                    : isSpore ? std::array<float, 4>{ 0.43f, 0.48f, 0.12f, 1.0f }
                    : std::array<float, 4>{ 0.40f, 0.26f, 0.10f, 1.0f };

            if (isThorn || isArcane)
            {
                pInstance->materialIndex = isHostile ? m_hostileSpellMaterial
                    : isThorn ? m_sapSpellMaterial : m_arcaneSpellMaterial;
            }
            else
            {
                const sShapeModelDesc& materialModel = isSpore ? m_sporecapModel : m_enemy03Model;
                const size_t materialSlot = isSpore ? 0 : 2;
                pInstance->materialIndex = materialModel.materialIndices.size() > materialSlot ? materialModel.materialIndices[materialSlot] : 0;
            }

            const MeshHandle mesh = isArcane ? m_icoSphereMesh : isThorn ? m_coneMesh : m_sphereMesh;

            m_dynamicMeshInstances[mesh].push_back(pInstance);

            sProjectileVisual projectileVisual{};
            projectileVisual.id        = projectile.id;
            projectileVisual.pInstance = pInstance;
            projectileVisual.mesh      = mesh;

            if (isArcane || isThorn || isSpore)
            {
                projectileVisual.detailMesh = isSpore ? m_icoSphereMesh : isArcane ? m_crystalMesh : m_coneMesh;
                const size_t detailCount = isThorn ? 3 : projectileVisual.details.size();
                for (size_t detailIndex = 0; detailIndex < detailCount; ++detailIndex)
                {
                    sInstanceData*& rpDetail = projectileVisual.details[detailIndex];
                    rpDetail = m_pool.Create();
                    rpDetail->materialIndex = isHostile ? m_hostileSpellMaterial
                        : isArcane ? m_arcaneSpellMaterial : m_sapSpellMaterial;
                    rpDetail->color = isHostile ? std::array<float, 4>{ 0.88f, 0.57f, 0.24f, 1.0f }
                        : isArcane
                        ? std::array<float, 4>{ 0.62f, 0.86f, 0.95f, 1.0f }
                        : isPlayerSpore ? std::array<float, 4>{ 0.74f, 0.77f, 0.39f, 1.0f }
                        : std::array<float, 4>{ 0.62f, 0.78f, 0.48f, 1.0f };
                    m_dynamicMeshInstances[projectileVisual.detailMesh].push_back(rpDetail);
                }
            }

            if (isPlayerSpell || isThorn || isSpore || isArcane)
            {
                sLight light{};
                light.type      = sLightType::Point;
                light.color     = isHostile ? Math::cVec3f(0.88f, 0.48f, 0.15f) : isPlayerSpore
                    ? Math::cVec3f(0.48f, 0.67f, 0.22f)
                    : projectile.type == Gameplay::eProjectileType::PlayerCone
                        ? Math::cVec3f(0.58f, 0.76f, 0.22f)
                        : Math::cVec3f(0.12f, 0.65f, 0.9f);
                light.intensity = isSpore ? 1.2f : 3.0f;
                light.position  = projectile.position;
                light.radius    = 4.0f;

                projectileVisual.light = LightManager::CreateLight(light);
            }

            m_projectileVisuals.push_back(projectileVisual);

            visual = std::prev(m_projectileVisuals.end());
            instanceListChanged = true;
        }

        if (projectile.type == Gameplay::eProjectileType::EnemyReflected)
            visual->pInstance->color = { 1.0f, 0.42f, 0.08f, 1.0f };
        else if (projectile.type == Gameplay::eProjectileType::PlayerReflected)
            visual->pInstance->color = { 0.32f, 0.72f, 1.0f, 1.0f };

        if (isReflected)
        {
            visual->pInstance->materialIndex = isHostile ? m_hostileSpellMaterial
                : isThorn ? m_sapSpellMaterial : m_arcaneSpellMaterial;
            for (sInstanceData* pDetail : visual->details)
            {
                if (pDetail == nullptr)
                    continue;

                pDetail->materialIndex = isHostile ? m_hostileSpellMaterial : isArcane ? m_arcaneSpellMaterial : m_sapSpellMaterial;
                pDetail->color = isHostile ? std::array<float, 4>{ 0.88f, 0.57f, 0.24f, 1.0f }
                    : std::array<float, 4>{ 0.62f, 0.86f, 0.72f, 1.0f };
            }
        }

        const bool isMushroom = projectile.type == Gameplay::eProjectileType::EnemySpore
            || projectile.type == Gameplay::eProjectileType::PlayerSpore;
        if (isMushroom && visual->pStem == nullptr)
        {
            visual->pStem = m_pool.Create();
            visual->pStem->materialIndex = visual->pInstance->materialIndex;
            visual->pStem->color = { 0.8f, 0.85f, 0.5f, 1.0f };
            m_dynamicMeshInstances[m_cylinderMesh].push_back(visual->pStem);
            instanceListChanged = true;
        }
        sTransform transform{};
        transform.position = projectile.position;

        if (isMushroom && projectile.areaActive)
        {
            transform.scale = { 0.0f, 0.0f, 0.0f };
        }
        else if (projectile.areaActive)
        {
            const float pulse = 1.0f + std::sin(projectile.areaAge * 9.0f) * 0.08f;
            transform.scale = { projectile.radius, 0.22f * pulse, projectile.radius };
            transform.rotation = { 0.0f, projectile.areaAge * 0.3f, 0.0f };
            visual->pInstance->color = projectile.type == Gameplay::eProjectileType::EnemyShockwave
                ? std::array<float, 4>{ 0.85f, 0.46f, 0.12f, 1.0f }
                : std::array<float, 4>{ 0.38f, 0.75f * pulse, 0.12f, 1.0f };
        }
        else if (isArcane)
        {
            const float horizontalLength = std::sqrt(projectile.direction.x() * projectile.direction.x()
                + projectile.direction.z() * projectile.direction.z());
            transform.rotation = { std::atan2(horizontalLength, projectile.direction.y()),
                std::atan2(projectile.direction.x(), projectile.direction.z()), 0.0f };
            transform.scale = { projectile.visualScale, projectile.visualScale, projectile.visualScale };
        }
        else if (projectile.type == Gameplay::eProjectileType::PlayerSpore
            || projectile.type == Gameplay::eProjectileType::EnemySpore)
        {
            const float horizontal = std::sqrt(projectile.direction.x() * projectile.direction.x()
                + projectile.direction.z() * projectile.direction.z());
            transform.rotation = { std::clamp(-std::atan2(projectile.direction.y(), horizontal) * 0.65f, -0.65f, 0.8f),
                std::atan2(projectile.direction.x(), projectile.direction.z()), std::sin(projectile.flightAge * 7.0f) * 0.12f };
            transform.scale = { 1.0f, 1.0f, 1.0f };
        }
        else
        {
            const float horizontalLength = std::sqrt(projectile.direction.x() * projectile.direction.x()
                + projectile.direction.z() * projectile.direction.z());

            const float pitch       = std::atan2(horizontalLength, projectile.direction.y());
            const float yaw         = std::atan2(projectile.direction.x(), projectile.direction.z());
            // All thorn owners share the same proportions, including after reflection.
            const float scale = isThorn ? projectile.visualScale : 1.0f;

            transform.rotation = { pitch, yaw, 0.0f };
            transform.scale = { 0.22f * scale, 1.5f * scale, 0.22f * scale };
        }

        const cMatrix4x4f projectileMatrix = CreateTransformMatrix(transform);
        visual->pInstance->worldMatrix = projectileMatrix;
        if (isArcane)
        {
            const float pulse = 1.0f + 0.035f * std::sin(projectile.flightAge * 12.0f);
            sTransform core{};
            core.rotation = { projectile.flightAge * 1.8f, projectile.flightAge * 2.5f, 0.0f };
            core.scale = { 0.72f * pulse, 0.84f * pulse, 0.72f * pulse };
            visual->pInstance->worldMatrix = CreateTransformMatrix(core) * projectileMatrix;
        }
        if (visual->pStem != nullptr)
        {
            sTransform stem{};
            stem.position = { 0.0f, -0.11f, 0.0f };
            stem.scale = { 0.18f, 0.38f, 0.18f };
            
            sTransform cap{};
            cap.position = { 0.0f, 0.15f, 0.0f };
            cap.scale = { 0.7f, 0.34f, 0.7f };
            
            visual->pStem->worldMatrix      = CreateTransformMatrix(stem) * projectileMatrix;
            visual->pInstance->worldMatrix  = CreateTransformMatrix(cap)  * projectileMatrix;
        }

        for (size_t detailIndex = 0; detailIndex < visual->details.size(); ++detailIndex)
        {
            sInstanceData* pDetail = visual->details[detailIndex];
            if (pDetail == nullptr)
                continue;

            const float phase = static_cast<float>(detailIndex) * 1.0471975512f;
            sTransform detail{};
            if (isMushroom)
            {
                detail.position = { std::cos(phase) * 0.23f, 0.275f, std::sin(phase) * 0.23f };
                detail.scale = { 0.105f, 0.05f, 0.09f };
            }
            else if (isArcane)
            {
                if (detailIndex < 4)
                {
                    const float orbit = static_cast<float>(detailIndex) * 1.57079632679f + projectile.flightAge * 7.0f;
                    detail.position = { std::cos(orbit) * 0.42f, std::sin(orbit * 2.0f) * 0.12f, std::sin(orbit) * 0.42f };
                    detail.rotation = { 0.35f, orbit, 0.4f };
                    detail.scale = { 0.10f, 0.24f, 0.10f };
                }
                else
                {
                    const float tailIndex = static_cast<float>(detailIndex - 4);
                    detail.position = { 0.0f, -0.55f - tailIndex * 0.40f, 0.0f };
                    detail.scale = { 0.18f - tailIndex * 0.07f, 0.55f - tailIndex * 0.15f, 0.18f - tailIndex * 0.07f };
                }
                pDetail->color = isHostile ? std::array<float, 4>{ 0.94f, 0.55f, 0.24f, 1.0f }
                    : detailIndex % 2 == 0 ? std::array<float, 4>{ 0.74f, 0.60f, 1.0f, 1.0f }
                    : std::array<float, 4>{ 0.66f, 0.94f, 1.0f, 1.0f };
            }
            else if (isThorn)
            {
                if (detailIndex == 0)
                {
                    // Close the broad base with a short tapered tail; the forward tip stays bare.
                    detail.position = { 0.0f, -0.68f, 0.0f };
                    detail.rotation = { 3.14159265359f, 0.0f, 0.0f };
                    detail.scale = { 1.0f, 0.36f, 1.0f };
                }
                else
                {
                    const float side = detailIndex == 1 ? -1.0f : 1.0f;
                    detail.position = { side * 0.65f, -0.46f, 0.0f };
                    detail.rotation = { 0.0f, 0.0f, side * 2.7f };
                    detail.scale = { 0.32f, 0.24f, 0.32f };
                }
            }
            else
            {
                detail.position = { std::cos(phase) * 0.65f, -0.25f + 0.1f * static_cast<float>(detailIndex % 3), std::sin(phase) * 0.65f };
                detail.rotation = { 0.8f * std::sin(phase), 0.0f, -0.8f * std::cos(phase) };
                detail.scale    = { 0.40f, 0.42f, 0.40f };
            }
            pDetail->worldMatrix = CreateTransformMatrix(detail) * projectileMatrix;
        }

        if (sLight* pLight = LightManager::TryGetLight(visual->light))
        {
            pLight->position = projectile.position;
            if (isReflected)
                pLight->color = isHostile ? cVec3f(0.88f, 0.48f, 0.15f) : cVec3f(0.32f, 0.72f, 1.0f);
            if (projectile.channeling)
                pLight->intensity = 1.0f + projectile.visualScale * 0.6f + 0.2f * std::sin(m_playerChannelTime * 12.0f);
        }
    }

    auto visual = m_projectileVisuals.begin();
    while (visual != m_projectileVisuals.end())
    {
        if (activeIds.contains(visual->id))
        {
            ++visual;
            continue;
        }

        std::vector<sInstanceData*>& meshInstances = m_dynamicMeshInstances[visual->mesh];
        std::erase(meshInstances, visual->pInstance);
        LightManager::DestroyLight(visual->light);
        if (visual->pStem != nullptr)
        {
            std::erase(m_dynamicMeshInstances[m_cylinderMesh], visual->pStem);
            m_pool.Destroy(visual->pStem);
        }

        m_particleSystem.StopEmitter(visual->sporeEmitter);
        m_particleSystem.StopEmitter(visual->skullEmitter);
        m_particleSystem.StopEmitter(visual->trailEmitter);
        for (sInstanceData* pDetail : visual->details)
        {
            if (pDetail == nullptr)
                continue;

            std::erase(m_dynamicMeshInstances[visual->detailMesh], pDetail);
            m_pool.Destroy(pDetail);
        }
        
        m_pool.Destroy(visual->pInstance);
        
        visual = m_projectileVisuals.erase(visual);
        
        instanceListChanged = true;
    }

    if (instanceListChanged)
        m_dynamicInstanceListDirty = true;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateProjectileEffects(float _deltaTime)
{
    using namespace Engine::GFX;
    using Engine::Math::cVec3f;

    m_spellVisualTime = std::fmod(m_spellVisualTime + _deltaTime, 628.318530718f);

    sParticleDefinition dashTrail{};
    dashTrail.spawnRate     = 130.0f;
    dashTrail.lifetime      = 0.18f;
    dashTrail.startSize     = 0.085f;
    dashTrail.endSize       = 0.01f;
    dashTrail.speed         = 0.05f;
    dashTrail.spread        = 0.35f;
    dashTrail.acceleration  = { 0.0f, -0.6f, 0.0f };
    dashTrail.startColor    = { 0.66f, 0.92f, 0.63f, 0.8f };
    dashTrail.endColor      = { 0.28f, 0.58f, 0.32f, 0.0f };

    const cVec3f dashPosition = m_playerController.GetPosition() + cVec3f(0.0f, 0.65f, 0.0f);
    if (m_playerDashTime > 0.0f && !m_inventoryOpen && !m_runState.HasPendingAugmentSelection())
    {
        if (!m_particleSystem.IsAlive(m_playerDashEmitter))
        {
            m_playerDashEmitter = m_particleSystem.CreateEmitter(dashTrail, dashPosition);
            m_particleSystem.Burst(dashTrail, dashPosition, 18);
        }
        m_particleSystem.SetPosition(m_playerDashEmitter, dashPosition);
    }
    else if (m_particleSystem.IsAlive(m_playerDashEmitter))
    {
        m_particleSystem.StopEmitter(m_playerDashEmitter);
        if (m_playerDashTime <= 0.0f)
            m_particleSystem.Burst(dashTrail, dashPosition, 12);
    }

    const auto poisonDefinition = [](bool _player)
    {
        sParticleDefinition definition{};
        definition.appearance   = eParticleAppearance::Facet;
        definition.spawnRate    = 65.0f;
        definition.lifetime     = 0.22f;
        definition.startSize    = 0.085f;
        definition.endSize      = 0.012f;
        definition.speed        = 0.025f;
        definition.spread       = 0.04f;
        definition.startColor   = _player ? std::array<float, 4>{ 0.30f, 0.95f, 0.16f, 0.82f } : std::array<float, 4>{ 0.72f, 0.90f, 0.08f, 0.88f };
        definition.endColor     = definition.startColor;
        definition.endColor[3]  = 0.0f;

        return definition;
    };

    m_particleSystem.BeginSurfaces();

    for (const Gameplay::sProjectile& projectile : m_projectileManager.GetProjectiles())
    {
        auto visual = std::find_if(m_projectileVisuals.begin(), m_projectileVisuals.end(), [&](const sProjectileVisual& _rVisual)
        {
            return _rVisual.id == projectile.id;
        });

        if (visual == m_projectileVisuals.end())
            continue;

        const bool isReflected = projectile.type == Gameplay::eProjectileType::EnemyReflected
            || projectile.type == Gameplay::eProjectileType::PlayerReflected;
        const bool isArcane = projectile.type == Gameplay::eProjectileType::PlayerSphere
            || (isReflected && !projectile.reflectedCone);
        const bool isThorn = projectile.type == Gameplay::eProjectileType::PlayerCone
            || projectile.type == Gameplay::eProjectileType::EnemyCone || (isReflected && projectile.reflectedCone);
        const bool isHostile = projectile.type == Gameplay::eProjectileType::EnemyCone
            || projectile.type == Gameplay::eProjectileType::EnemyReflected;

        if (isArcane || isThorn)
        {
            if (visual->trailHostile != isHostile || visual->wasChanneling != projectile.channeling)
                m_particleSystem.StopEmitter(visual->trailEmitter);
            visual->trailHostile = isHostile;
            sParticleDefinition trail{};
            trail.appearance = eParticleAppearance::Facet;
            trail.spawnRate  = projectile.channeling ? 48.0f : isArcane ? 85.0f : 55.0f;
            trail.lifetime   = 0.18f;
            trail.startSize  = isArcane ? 0.10f : 0.045f;
            trail.endSize    = 0.008f;
            trail.speed      = 0.02f;
            trail.spread     = 0.07f;
            trail.startColor = isHostile ? std::array<float, 4>{ 0.88f, 0.48f, 0.15f, 0.7f }
                : isArcane ? std::array<float, 4>{ 0.42f, 0.82f, 1.0f, 0.8f }
                : std::array<float, 4>{ 0.62f, 0.86f, 0.34f, 0.75f };

            trail.endColor = isArcane && !isHostile ? std::array<float, 4>{ 0.55f, 0.30f, 0.95f, 0.0f }
                : trail.startColor;
            trail.endColor[3] = 0.0f;

            if (visual->wasChanneling && !projectile.channeling)
            {
                sParticleDefinition release = trail;
                release.speed     = 0.05f;
                release.spread    = 2.8f;
                release.startSize = 0.075f;
                m_particleSystem.Burst(release, projectile.position, 16);
            }
            visual->wasChanneling   = projectile.channeling;
            cVec3f emissionPosition = projectile.position - projectile.direction * ((isThorn ? 1.25f : 0.4f) * projectile.visualScale);

            if (projectile.channeling)
            {
                const float phase = m_playerChannelTime * 10.0f;
                const cVec3f side(projectile.direction.z(), 0.0f, -projectile.direction.x());
                const cVec3f right = side.isZero() ? cVec3f(1.0f, 0.0f, 0.0f) : side.normalized();
                emissionPosition += (right * std::cos(phase) + cVec3f(0.0f, std::sin(phase), 0.0f))
                    * (0.12f + 0.04f * projectile.visualScale);
            }
            if (!m_particleSystem.IsAlive(visual->trailEmitter))
            {
                visual->trailEmitter = m_particleSystem.CreateEmitter(trail, emissionPosition);
                m_particleSystem.Burst(trail, emissionPosition, isArcane ? 4 : 8);
                if (isArcane)
                {
                    sParticleDefinition flash = trail;
                    flash.appearance = eParticleAppearance::Soft;
                    flash.lifetime   = 0.10f;
                    flash.startSize  = 0.28f;
                    flash.endSize    = 0.02f;
                    flash.spread     = 0.0f;
                    m_particleSystem.Burst(flash, projectile.position, 1);
                }
            }
            m_particleSystem.SetPosition(visual->trailEmitter, emissionPosition);
            continue;
        }
        m_particleSystem.StopEmitter(visual->trailEmitter);

        if (projectile.type != Gameplay::eProjectileType::EnemySpore && projectile.type != Gameplay::eProjectileType::PlayerSpore)
            continue;

        const auto chunk = std::make_pair(static_cast<int>(std::floor(projectile.position.x() / World::c_chunkSize + 0.5f)),
            static_cast<int>(std::floor(projectile.position.z() / World::c_chunkSize + 0.5f)));

        if (!World::WorldGenerator::GetLoadedChunks().contains(chunk))
        {
            m_particleSystem.StopEmitter(visual->sporeEmitter, true);
            m_particleSystem.StopEmitter(visual->skullEmitter, true);
            continue;
        }

        if (visual->emittingArea != projectile.areaActive)
        {
            m_particleSystem.StopEmitter(visual->sporeEmitter);
            visual->emittingArea = projectile.areaActive;
        }
        const bool player = projectile.type == Gameplay::eProjectileType::PlayerSpore;
        if (!m_particleSystem.IsAlive(visual->sporeEmitter))
        {
            sParticleDefinition definition = poisonDefinition(player);
            if (projectile.areaActive)
            {
                definition.spawnRate     = 26.0f;
                definition.startSize     = 0.045f;
                definition.endSize       = 0.018f;
                definition.lifetime      = 0.75f;
                definition.speed         = 0.24f;
                definition.spread        = 0.08f;
            }
            visual->sporeEmitter = m_particleSystem.CreateEmitter(definition, projectile.position);
        }
        const cVec3f sporePosition = projectile.areaActive ? projectile.position
            : projectile.position - projectile.direction * 0.25f + cVec3f(0.0f, -0.08f, 0.0f);
        m_particleSystem.SetPosition(visual->sporeEmitter, sporePosition);
        if (projectile.areaActive)
        {
            if (!m_particleSystem.IsAlive(visual->skullEmitter))
            {
                sParticleDefinition skulls = poisonDefinition(player);
                skulls.appearance = eParticleAppearance::Skull;
                skulls.spawnRate  = 3.0f;
                skulls.lifetime   = 1.4f;
                skulls.startSize  = 0.18f;
                skulls.endSize    = 0.25f;
                skulls.speed      = 0.55f;
                skulls.spread     = 0.10f;
                visual->skullEmitter = m_particleSystem.CreateEmitter(skulls, projectile.position);
            }
            std::array<sParticleSurface, Gameplay::sProjectile::c_maxGroundSamples> surfaces{};
            std::array<float, 4> color = poisonDefinition(player).startColor;
            color[3] = (player ? 0.76f : 0.82f) * std::clamp(projectile.lifetime / 0.3f, 0.0f, 1.0f);
            for (size_t index = 0; index < projectile.groundSampleCount; ++index)
            {
                surfaces[index] = { projectile.groundSamples[index], projectile.groundNormals[index], projectile.GetGroundSampleRadius(index) };
                surfaces[index].tileHalfExtent = projectile.areaRadius / 9.0f;
                surfaces[index].areaClip = { projectile.position.x(), projectile.position.z(), projectile.radius };
                m_particleSystem.AddSurface(surfaces[index], color, projectile.areaAge);
            }
            m_particleSystem.SetSurfaces(visual->sporeEmitter, { surfaces.data(), projectile.groundSampleCount });
            m_particleSystem.SetSurfaces(visual->skullEmitter, { surfaces.data(), projectile.groundSampleCount });
        }
    }

    m_particleSystem.Update(_deltaTime);

    // Explicit events preserve impacts even when a projectile is born and expires in one update.
    for (const Gameplay::sProjectileImpactEvent& impact : m_projectileManager.GetImpactEvents())
    {
        const auto chunk = std::make_pair(static_cast<int>(std::floor(impact.position.x() / World::c_chunkSize + 0.5f)),
            static_cast<int>(std::floor(impact.position.z() / World::c_chunkSize + 0.5f)));

        if (!World::WorldGenerator::GetLoadedChunks().contains(chunk))
            continue;

        if (impact.type == Gameplay::eProjectileType::PlayerSphere || impact.type == Gameplay::eProjectileType::PlayerCone
            || impact.type == Gameplay::eProjectileType::PlayerReflected || impact.type == Gameplay::eProjectileType::EnemyCone
            || impact.type == Gameplay::eProjectileType::EnemyReflected)
        {
            sParticleDefinition sparks{};
            sparks.speed        = 0.15f;
            sparks.spread       = 4.0f;
            sparks.lifetime     = 0.2f;
            sparks.startSize    = 0.075f;
            sparks.endSize      = 0.01f;
            sparks.acceleration = { 0.0f, -2.0f, 0.0f };
            sparks.startColor = impact.type == Gameplay::eProjectileType::EnemyCone || impact.type == Gameplay::eProjectileType::EnemyReflected
                ? std::array<float, 4>{ 0.95f, 0.57f, 0.20f, 0.85f }
                : impact.type == Gameplay::eProjectileType::PlayerSphere
                ? std::array<float, 4>{ 0.4f, 0.8f, 1.0f, 0.85f }
                : std::array<float, 4>{ 0.76f, 0.83f, 0.38f, 0.85f };

            const bool isOrbImpact = impact.type == Gameplay::eProjectileType::PlayerSphere;
            if (isOrbImpact)
            {
                sparks.appearance   = eParticleAppearance::Facet;
                sparks.spread       = 5.5f;
                sparks.lifetime     = 0.28f;
                sparks.startSize    = 0.10f;
                sparks.acceleration = { 0.0f, -0.6f, 0.0f };
            }
            sparks.endColor = isOrbImpact ? std::array<float, 4>{ 0.55f, 0.30f, 0.95f, 0.0f } : sparks.startColor;
            sparks.endColor[3] = 0.0f;
            m_particleSystem.Burst(sparks, impact.position, isOrbImpact ? 14 : 18);
            if (isOrbImpact)
            {
                sParticleDefinition flash = sparks;
                flash.appearance   = eParticleAppearance::Soft;
                flash.speed        = 0.0f;
                flash.spread       = 0.0f;
                flash.acceleration = {};
                flash.lifetime     = 0.13f;
                flash.startSize    = 0.18f;
                flash.endSize      = 0.60f;
                flash.startColor   = { 0.75f, 0.94f, 1.0f, 0.9f };
                m_particleSystem.Burst(flash, impact.position, 1);
            }
            continue;
        }

        sParticleDefinition definition = poisonDefinition(impact.type == Gameplay::eProjectileType::PlayerSpore);
        definition.speed        = 0.8f;
        definition.spread       = 2.5f;
        definition.startSize    = 0.1f;
        definition.endSize      = 0.35f;
        definition.lifetime     = 0.85f;

        m_particleSystem.Burst(definition, impact.position, 36);
    }
    m_projectileManager.ClearImpactEvents();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::SyncLootRenderInstances()
{
    using namespace Engine::GFX;

    if (m_lootRevision == m_lootManager.GetRevision())
        return;

    for (const sLootVisual& visual : m_lootVisuals)
    {
        std::vector<sInstanceData*>& meshInstances = m_dynamicMeshInstances[visual.mesh];
        std::erase(meshInstances, visual.pInstance);
        m_pool.Destroy(visual.pInstance);
    }
    m_lootVisuals.clear();

    for (const Gameplay::sLootDrop& drop : m_lootManager.GetDrops())
    {
        sInstanceData* pInstance = m_pool.Create();

        pInstance->color = drop.item.rarity == Gameplay::sItemRarity::Rare
            ? std::array<float, 4>{ 0.32f, 0.62f, 1.0f, 1.0f }
            : drop.item.rarity == Gameplay::sItemRarity::Legendary
                ? std::array<float, 4>{ 1.0f, 0.70f, 0.12f, 1.0f }
                : std::array<float, 4>{ 0.9f, 0.94f, 1.0f, 1.0f };

        pInstance->materialIndex = m_playerSphereMaterial;

        GFX::sTransform transform{};
        transform.position  = drop.position + Math::cVec3f(0.0f, 0.35f, 0.0f);
        transform.scale     = { 0.18f, 0.18f, 0.18f };

        pInstance->worldMatrix = CreateTransformMatrix(transform);

        m_dynamicMeshInstances[m_sphereMesh].push_back(pInstance);
        m_lootVisuals.push_back({ pInstance, m_sphereMesh });
    }

    m_lootRevision = m_lootManager.GetRevision();
    m_dynamicInstanceListDirty = true;
}

// -------------------------------------------------------------------------------------------------------------------------

