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

    const cMatrix4x4f playerMatrix = CreateTransformMatrix(playerTransform);

    float attackWeight = 1.0f - std::abs(m_playerAttackTime - 0.2f) / 0.2f;

    attackWeight = std::clamp(attackWeight, 0.0f, 1.0f);
    attackWeight = attackWeight * attackWeight * (3.0f - 2.0f * attackWeight);

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

        const bool hasReflectionAura = pEnemy->state != Gameplay::eEnemyState::Dead && pEnemy->projectileReflectionAuraTime > 0.0f;
        if (hasReflectionAura)
        {
            sLight auraLight{};
            auraLight.type      = sLightType::Point;
            auraLight.color     = { 0.38f, 0.95f, 0.16f };
            auraLight.intensity = 3.0f;
            auraLight.position  = pEnemy->position + cVec3f(0.0f, 1.4f * pEnemy->scale, 0.0f);
            auraLight.radius    = 4.0f * pEnemy->scale;

            if (visual.auraLight == c_invalidLightHandle)
                visual.auraLight = LightManager::CreateLight(auraLight);
            else
                LightManager::UpdateLight(visual.auraLight, auraLight);

            visual.auraShieldPhase = std::fmod(visual.auraShieldPhase + _deltaTime * 2.4f, 6.28318530718f);
            for (size_t shieldIndex = 0; shieldIndex < visual.auraShields.size(); ++shieldIndex)
            {
                sInstanceData*& rpShield = visual.auraShields[shieldIndex];
                if (rpShield == nullptr)
                {
                    rpShield = m_pool.Create();
                    rpShield->color = { 0.38f, 0.95f, 0.16f, 1.0f };
                    rpShield->materialIndex = m_playerSphereMaterial;
                    m_dynamicMeshInstances[m_beveledCubeMesh].push_back(rpShield);
                    m_dynamicInstanceListDirty = true;
                }

                const float angle = visual.auraShieldPhase + static_cast<float>(shieldIndex) * 1.57079632679f;
                sTransform shieldTransform{};
                shieldTransform.position = pEnemy->position + cVec3f(
                    std::cos(angle) * 1.8f * pEnemy->scale,
                    (1.4f + std::sin(visual.auraShieldPhase * 2.0f + static_cast<float>(shieldIndex)) * 0.12f) * pEnemy->scale,
                    std::sin(angle) * 1.8f * pEnemy->scale);

                const cVec3f playerPosition = m_playerController.GetPosition();
                const cVec3f toPlayer(playerPosition.x() - shieldTransform.position.x(), 0.0f,
                    playerPosition.z() - shieldTransform.position.z());
                shieldTransform.rotation = { 0.0f, std::atan2(toPlayer.x(), toPlayer.z()), 0.0f };
                shieldTransform.scale    = { 0.38f * pEnemy->scale, 0.55f * pEnemy->scale, 0.08f * pEnemy->scale };
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

        auto visual = std::find_if(m_projectileVisuals.begin(), m_projectileVisuals.end(), [&projectile](const sProjectileVisual& _rVisual)
        {
            return _rVisual.id == projectile.id;
        });

        if (visual == m_projectileVisuals.end())
        {
            sInstanceData* pInstance = m_pool.Create();
            const bool isEnemyReflected = projectile.type == Gameplay::eProjectileType::EnemyReflected;
            const bool isPlayerReflected = projectile.type == Gameplay::eProjectileType::PlayerReflected;
            const bool isPlayerSpell = projectile.type == Gameplay::eProjectileType::PlayerSphere
                || projectile.type == Gameplay::eProjectileType::PlayerCone
                || projectile.type == Gameplay::eProjectileType::PlayerSpore
                || isPlayerReflected;
            const bool isPlayerSpore = projectile.type == Gameplay::eProjectileType::PlayerSpore;
            const bool isSpore = isPlayerSpore || projectile.type == Gameplay::eProjectileType::EnemySpore;

            pInstance->color = isEnemyReflected
                ? std::array<float, 4>{ 1.0f, 0.42f, 0.08f, 1.0f }
                : isPlayerReflected
                ? std::array<float, 4>{ 0.32f, 0.72f, 1.0f, 1.0f }
                : isPlayerSpore
                ? std::array<float, 4>{ 0.35f, 0.95f, 0.25f, 1.0f }
                : isPlayerSpell
                    ? projectile.type == Gameplay::eProjectileType::PlayerCone
                        ? std::array<float, 4>{ 0.95f, 0.52f, 0.12f, 1.0f }
                        : std::array<float, 4>{ 0.20f, 0.55f, 1.0f, 1.0f }
                    : projectile.type == Gameplay::eProjectileType::EnemyShockwave
                        ? std::array<float, 4>{ 0.85f, 0.46f, 0.12f, 1.0f }
                    : isSpore ? std::array<float, 4>{ 0.48f, 0.85f, 0.12f, 1.0f }
                    : std::array<float, 4>{ 0.35f, 1.0f, 0.18f, 1.0f };

            if (isPlayerSpell && !isSpore)
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
                || ((isEnemyReflected || isPlayerReflected) && projectile.reflectedCone)
                ? m_coneMesh
                : m_sphereMesh;

            m_dynamicMeshInstances[mesh].push_back(pInstance);

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
                        : Math::cVec3f(0.20f, 0.55f, 1.0f);
                light.intensity = isPlayerSpore ? 2.0f : 10.0f;
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
        else if (projectile.type == Gameplay::eProjectileType::PlayerSphere)
        {
            transform.rotation = { 0.0f, 0.0f, 0.0f };
            transform.scale = { 0.42f * projectile.visualScale, 0.42f * projectile.visualScale, 0.42f * projectile.visualScale };
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
            const float pitch = std::atan2(horizontalLength, projectile.direction.y());
            const float yaw = std::atan2(projectile.direction.x(), projectile.direction.z());
            const float radiusScale = projectile.type == Gameplay::eProjectileType::EnemyCone
                ? projectile.radius / 0.3f
                : projectile.type == Gameplay::eProjectileType::PlayerCone
                ? projectile.visualScale
                : 1.0f;
            const float lengthScale = projectile.type == Gameplay::eProjectileType::PlayerCone
                ? projectile.visualScale
                : 1.0f;

            transform.rotation = { pitch, yaw, 0.0f };
            transform.scale = { 0.14f * radiusScale, 0.65f * lengthScale, 0.14f * radiusScale };
        }

        visual->pInstance->worldMatrix = CreateTransformMatrix(transform);
        if (visual->pStem != nullptr)
        {
            sTransform stem{};
            stem.position = { 0.0f, -0.11f, 0.0f };
            stem.scale = { 0.18f, 0.38f, 0.18f };
            
            sTransform cap{};
            cap.position = { 0.0f, 0.15f, 0.0f };
            cap.scale = { 0.7f, 0.34f, 0.7f };
            
            const cMatrix4x4f mushroomMatrix = CreateTransformMatrix(transform);

            visual->pStem->worldMatrix = mushroomMatrix * CreateTransformMatrix(stem);
            visual->pInstance->worldMatrix = mushroomMatrix * CreateTransformMatrix(cap);
        }

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

        std::vector<sInstanceData*>& meshInstances = m_dynamicMeshInstances[visual->mesh];
        std::erase(meshInstances, visual->pInstance);
        LightManager::DestroyLight(visual->light);
        if (visual->pStem != nullptr)
        {
            std::erase(m_dynamicMeshInstances[m_cylinderMesh], visual->pStem);
            m_pool.Destroy(visual->pStem);
        }

        m_particleSystem.StopEmitter(visual->sporeEmitter);
        m_particleSystem.StopEmitter(visual->bubbleEmitter);
        
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

    const auto poisonDefinition = [](bool _player)
    {
        sParticleDefinition definition{};
        definition.spawnRate    = 45.0f;
        definition.lifetime     = 0.55f;
        definition.startSize    = 0.035f;
        definition.endSize      = 0.11f;
        definition.speed        = 0.08f;
        definition.spread       = 0.15f;
        definition.startColor   = _player ? std::array<float, 4>{ 0.12f, 0.85f, 0.38f, 0.65f } : std::array<float, 4>{ 0.4f, 0.9f, 0.015f, 0.65f };
        definition.endColor     = definition.startColor;
        definition.endColor[3]  = 0.0f;

        return definition;
    };

    m_particleSystem.BeginSurfaces();

    for (const Gameplay::sProjectile& projectile : m_projectileManager.GetProjectiles())
    {
        if (projectile.type != Gameplay::eProjectileType::EnemySpore && projectile.type != Gameplay::eProjectileType::PlayerSpore)
            continue;

        auto visual = std::find_if(m_projectileVisuals.begin(), m_projectileVisuals.end(), [&](const sProjectileVisual& _rVisual)
        {
            return _rVisual.id == projectile.id;
        });

        if (visual == m_projectileVisuals.end())
            continue;

        const auto chunk = std::make_pair(static_cast<int>(std::floor(projectile.position.x() / World::c_chunkSize + 0.5f)),
            static_cast<int>(std::floor(projectile.position.z() / World::c_chunkSize + 0.5f)));

        if (!World::WorldGenerator::GetLoadedChunks().contains(chunk))
        {
            m_particleSystem.StopEmitter(visual->sporeEmitter, true);
            m_particleSystem.StopEmitter(visual->bubbleEmitter, true);
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
                definition.appearance    = eParticleAppearance::Vapor;
                definition.spawnRate     = 40.0f;
                definition.startSize     = 0.35f;
                definition.endSize       = 0.85f;
                definition.lifetime      = 2.0f;
                definition.speed         = 0.13f;
                definition.startColor[3] = 0.32f;
            }
            visual->sporeEmitter = m_particleSystem.CreateEmitter(definition, projectile.position);
        }
        m_particleSystem.SetPosition(visual->sporeEmitter, projectile.position);
        if (projectile.areaActive)
        {
            if (!m_particleSystem.IsAlive(visual->bubbleEmitter))
            {
                sParticleDefinition bubbles = poisonDefinition(player);
                bubbles.appearance    = eParticleAppearance::Bubble;
                bubbles.spawnRate     = 8.0f;
                bubbles.startSize     = 0.045f;
                bubbles.endSize       = 0.13f;
                bubbles.lifetime      = 1.0f;
                bubbles.speed         = 0.11f;
                bubbles.spread        = 0.05f;
                visual->bubbleEmitter = m_particleSystem.CreateEmitter(bubbles, projectile.position);
            }
            std::array<sParticleSurface, Gameplay::sProjectile::c_maxGroundSamples> surfaces{};
            std::array<float, 4> color = poisonDefinition(player).startColor;
            color[3] = 0.88f;
            for (size_t index = 0; index < projectile.groundSampleCount; ++index)
            {
                surfaces[index] = { projectile.groundSamples[index], projectile.groundNormals[index], projectile.GetGroundSampleRadius(index) };
                surfaces[index].tileHalfExtent = projectile.areaRadius / 9.0f;
                surfaces[index].areaClip = { projectile.position.x(), projectile.position.z(), projectile.radius };
                m_particleSystem.AddSurface(surfaces[index], color, projectile.areaAge);
            }
            m_particleSystem.SetSurfaces(visual->sporeEmitter, { surfaces.data(), projectile.groundSampleCount });
            m_particleSystem.SetSurfaces(visual->bubbleEmitter, { surfaces.data(), projectile.groundSampleCount });
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

        sParticleDefinition definition = poisonDefinition(impact.type == Gameplay::eProjectileType::PlayerSpore);
        definition.speed        = 0.8f;
        definition.spread       = 2.5f;
        definition.startSize    = 0.1f;
        definition.endSize      = 0.35f;
        definition.lifetime     = 0.85f;

        m_particleSystem.Burst(definition, impact.position, 36);

        definition.appearance    = eParticleAppearance::Vapor;
        definition.startSize     = 0.3f;
        definition.endSize       = 1.0f;
        definition.speed         = 0.35f;
        definition.spread        = 1.1f;
        definition.startColor[3] = 0.45f;

        m_particleSystem.Burst(definition, impact.position, 12);
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

