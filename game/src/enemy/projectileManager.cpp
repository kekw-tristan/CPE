#include "projectileManager.h"

#include "enemyManager.h"
#include "physics/collider.h"
#include "physics/collisionWorld.h"

#include <algorithm>
#include <cmath>

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{
    namespace
    {
        constexpr int c_sporeGroundSampleHalfExtent = 4;
        constexpr float c_sporeGroundSampleDivisor = 4.5f;

        void ActivateArea(sProjectile& _rProjectile)
        {
            _rProjectile.areaActive = true;
            _rProjectile.speed      = 0.0f;
            _rProjectile.areaAge    = 0.0f;
            _rProjectile.radius     = 0.2f;
            _rProjectile.lifetime   = _rProjectile.areaDuration;

            float groundHeight = 0.0f;
            if (Engine::Physics::CollisionWorld::FindGroundHeight(_rProjectile.position, _rProjectile.position.y(), groundHeight))
            {
                _rProjectile.position = { _rProjectile.position.x(), groundHeight + 0.2f, _rProjectile.position.z() };
            }

            if (_rProjectile.type != eProjectileType::EnemySpore && _rProjectile.type != eProjectileType::PlayerSpore)
                return;

            // Cache terrain/steps once at impact; visuals and damage share this footprint.
            const float spacing = _rProjectile.areaRadius / c_sporeGroundSampleDivisor;
            _rProjectile.groundSampleRadius = spacing * 0.75f;

            for (int z = -c_sporeGroundSampleHalfExtent; z <= c_sporeGroundSampleHalfExtent; ++z)
            {
                for (int x = -c_sporeGroundSampleHalfExtent; x <= c_sporeGroundSampleHalfExtent; ++x)
                {
                    const Engine::Math::cVec3f sample = _rProjectile.position + Engine::Math::cVec3f(x * spacing, 0.0f, z * spacing);

                    if (Engine::Physics::CollisionWorld::FindGroundHeight(sample, _rProjectile.position.y() + _rProjectile.areaRadius, groundHeight))
                    {
                        const size_t index = _rProjectile.groundSampleCount++;

                        _rProjectile.groundSamples[index] = { sample.x(), groundHeight, sample.z() };
                        
                        float heightX = groundHeight;
                        float heightZ = groundHeight;
                        
                        Engine::Physics::CollisionWorld::FindGroundHeight(sample + Engine::Math::cVec3f(0.1f, 0.0f, 0.0f), groundHeight + 0.3f, heightX);
                        Engine::Physics::CollisionWorld::FindGroundHeight(sample + Engine::Math::cVec3f(0.0f, 0.0f, 0.1f), groundHeight + 0.3f, heightZ);

                        // Tilt with slopes, but keep patches flat at abrupt step edges.
                        const float slopeX = std::abs(heightX - groundHeight) < 0.25f ? (heightX - groundHeight) * 10.0f : 0.0f;
                        const float slopeZ = std::abs(heightZ - groundHeight) < 0.25f ? (heightZ - groundHeight) * 10.0f : 0.0f;

                        _rProjectile.groundNormals[index] = Engine::Math::cVec3f(-slopeX, 1.0f, -slopeZ).normalized();
                    }
                }
            }
        }

        // Use the same world geometry as characters, with short steps to stop at the first contact.
        bool MoveProjectile(sProjectile& _rProjectile, float _deltaTime)
        {
            const Engine::Math::cVec3f velocity = _rProjectile.direction * _rProjectile.speed;
            const Engine::Math::cVec3f acceleration(0.0f, -_rProjectile.gravity, 0.0f);
            const Engine::Math::cVec3f movement = velocity * _deltaTime + acceleration * (0.5f * _deltaTime * _deltaTime);
            const Engine::Math::cVec3f nextVelocity = velocity + acceleration * _deltaTime;

            _rProjectile.speed = nextVelocity.length();
            _rProjectile.direction = nextVelocity.normalized();
            _rProjectile.flightAge += _deltaTime;
            
            Engine::Physics::sCapsuleCollider sphere{};
            sphere.center       = _rProjectile.position;
            sphere.radius       = 0.2f;
            sphere.halfHeight   = 0.0f;

            const Engine::Math::cVec3f expectedPosition = sphere.center + movement;

            _rProjectile.position = Engine::Physics::CollisionWorld::MoveCapsule(sphere, movement);
            bool hitWorld = Engine::Math::cVec3f::distanceSquared(expectedPosition, _rProjectile.position) > 0.00000001f;

            float groundHeight = 0.0f;
            if (Engine::Physics::CollisionWorld::FindGroundHeight(_rProjectile.position, std::max(sphere.center.y(), _rProjectile.position.y()) + sphere.radius, groundHeight)
                && _rProjectile.position.y() - sphere.radius <= groundHeight)
            {
                _rProjectile.position = { _rProjectile.position.x(), groundHeight + sphere.radius, _rProjectile.position.z() };
                hitWorld = true;
            }

            return hitWorld;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void AimMushroomThrow(sProjectileSpawnDesc& _rDesc, const Engine::Math::cVec3f& _rTarget)
    {
        const Engine::Math::cVec3f offset = _rTarget - _rDesc.position;
        const float distance = std::sqrt(offset.x() * offset.x() + offset.z() * offset.z());
        const float flightTime = std::clamp(distance / std::max(_rDesc.speed, 1.0f), 0.25f, 1.8f);
        _rDesc.gravity = 12.0f;
        const Engine::Math::cVec3f velocity = offset / flightTime
            + Engine::Math::cVec3f(0.0f, 0.5f * _rDesc.gravity * flightTime, 0.0f);
        _rDesc.speed = velocity.length();
        _rDesc.direction = velocity.normalized();
        _rDesc.lifetime = flightTime + 0.75f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float sProjectile::GetGroundSampleRadius(size_t _index) const
    {
        const Engine::Math::cVec3f offset = groundSamples[_index] - position;
        const float distance = std::sqrt(offset.x() * offset.x() + offset.z() * offset.z());
        return std::min(groundSampleRadius, std::max(0.0f, radius - distance));
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool sProjectile::ContainsGroundPoint(const Engine::Math::cVec3f& _rPosition) const
    {
        const Engine::Math::cVec3f offset = _rPosition - position;

        if (offset.x() * offset.x() + offset.z() * offset.z() > radius * radius)
            return false;

        for (size_t index = 0; index < groundSampleCount; ++index)
        {
            const Engine::Math::cVec3f local = _rPosition - groundSamples[index];

            const float halfExtent          = areaRadius / (2.0f * c_sporeGroundSampleDivisor);
            const float heightAboveGround   = local.dot(groundNormals[index]) / groundNormals[index].y();

            if (std::abs(local.x()) <= halfExtent && std::abs(local.z()) <= halfExtent
                && heightAboveGround >= -0.15f && heightAboveGround <= 0.65f)
            {
                return true;
            }
        }
        return false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnCone(const sProjectileSpawnDesc& _rDesc)
    {
        return Spawn(_rDesc, eProjectileType::EnemyCone);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnSpore(const sProjectileSpawnDesc& _rDesc)
    {
        return Spawn(_rDesc, eProjectileType::EnemySpore);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnShockwave(const sProjectileSpawnDesc& _rDesc)
    {
        const uint64_t id = Spawn(_rDesc, eProjectileType::EnemyShockwave);
        ActivateArea(m_projectiles.back());
        return id;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnPlayerSphere(const sProjectileSpawnDesc& _rDesc)
    {
        return Spawn(_rDesc, eProjectileType::PlayerSphere);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnPlayerChannelCone(const sProjectileSpawnDesc& _rDesc)
    {
        const uint64_t id = Spawn(_rDesc, eProjectileType::PlayerCone);
        m_projectiles.back().channeling = true;
        return id;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnPlayerCone(const sProjectileSpawnDesc& _rDesc)
    {
        return Spawn(_rDesc, eProjectileType::PlayerCone);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::SpawnPlayerSpore(const sProjectileSpawnDesc& _rDesc)
    {
        return Spawn(_rDesc, eProjectileType::PlayerSpore);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cProjectileManager::UpdatePlayerChannelCone(uint64_t _id, const Engine::Math::cVec3f& _rPosition,
                                                      const Engine::Math::cVec3f& _rDirection, float _radius, float _visualScale)
    {
        const auto projectile = std::find_if(m_projectiles.begin(), m_projectiles.end(), [_id](const sProjectile& _rProjectile)
        {
            return _rProjectile.id == _id && _rProjectile.channeling;
        });

        if (projectile == m_projectiles.end())
            return false;

        projectile->position    = _rPosition;
        projectile->direction   = _rDirection.normalized();
        projectile->radius      = _radius;
        projectile->visualScale = _visualScale;
        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cProjectileManager::ReleasePlayerChannelCone(uint64_t _id, const Engine::Math::cVec3f& _rDirection,
                                                       float _speed, float _damage, float _lifetime)
    {
        const auto projectile = std::find_if(m_projectiles.begin(), m_projectiles.end(), [_id](const sProjectile& _rProjectile)
        {
            return _rProjectile.id == _id && _rProjectile.channeling;
        });

        if (projectile == m_projectiles.end())
            return false;

        projectile->channeling = false;
        projectile->direction  = _rDirection.normalized();
        projectile->speed      = _speed;
        projectile->damage     = _damage;
        projectile->lifetime   = _lifetime;
        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::Spawn(const sProjectileSpawnDesc& _rDesc, eProjectileType _type)
    {
        sProjectile projectile{};

        projectile.id               = m_nextId++;
        projectile.position         = _rDesc.position;
        projectile.direction        = _rDesc.direction.normalized();
        projectile.speed            = _rDesc.speed;
        projectile.gravity          = std::max(0.0f, _rDesc.gravity);
        projectile.damage           = _rDesc.damage;
        projectile.lifetime         = _rDesc.lifetime;
        projectile.radius           = _rDesc.radius;
        projectile.visualScale      = _rDesc.visualScale;
        projectile.isAreaOfEffect   = _rDesc.isAreaOfEffect;
        projectile.areaRadius       = std::max(0.2f, _rDesc.areaRadius > 0.0f ? _rDesc.areaRadius : _rDesc.radius);
        projectile.areaDuration     = std::max(0.01f, _rDesc.areaDuration);
        projectile.areaGrowthTime   = std::max(0.01f, _rDesc.areaGrowthTime);
        projectile.piercesRemaining = std::max(0, _rDesc.pierces);
        projectile.type             = _type;

        m_projectiles.push_back(projectile);

        return projectile.id;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cProjectileManager::Update(float _deltaTime, const Engine::Math::cVec3f& _rPlayerPosition, cEnemyManager& _rEnemyManager,
                                    float _playerReflectionAuraTime, float _playerReflectionAuraRadius, float _playerReflectionDamageMultiplier)
    {
        m_impactEvents.clear();

        const auto activateArea = [&](sProjectile& _rProjectile)
        {
            ActivateArea(_rProjectile);
            if (_rProjectile.type == eProjectileType::EnemySpore || _rProjectile.type == eProjectileType::PlayerSpore)
                m_impactEvents.push_back({ _rProjectile.id, _rProjectile.position, _rProjectile.type });
        };

        const Engine::Math::cVec3f playerCenter = _rPlayerPosition + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f);

        for (sProjectile& projectile : m_projectiles)
        {
            if (projectile.channeling)
                continue;

            if (projectile.type == eProjectileType::EnemyCone && _playerReflectionAuraTime > 0.0f)
            {
                const Engine::Math::cVec3f auraCenter = _rPlayerPosition + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f);
                const float reflectionRadius = _playerReflectionAuraRadius + projectile.radius;
                if (Engine::Math::cVec3f::distanceSquared(projectile.position, auraCenter) <= reflectionRadius * reflectionRadius)
                {
                    projectile.type          = eProjectileType::PlayerReflected;
                    projectile.direction     = projectile.direction * -1.0f;
                    projectile.damage        *= _playerReflectionDamageMultiplier;
                    projectile.speed         *= 1.15f;
                    projectile.reflectedCone = true;
                }
            }

            bool isPlayerProjectile = projectile.type == eProjectileType::PlayerSphere
                || projectile.type == eProjectileType::PlayerCone
                || projectile.type == eProjectileType::PlayerSpore
                || projectile.type == eProjectileType::PlayerReflected;

            float remainingTime = std::max(0.0f, _deltaTime);
            while (remainingTime > 0.000001f && projectile.lifetime > 0.0f)
            {
                // Bound both travel distance and area growth, including during long frames.
                const float stepTime = std::min({ remainingTime, projectile.lifetime,
                    1.0f / 60.0f, 0.1f / std::max(projectile.speed, 0.1f) });
                remainingTime -= stepTime;
                projectile.lifetime = std::max(0.0f, projectile.lifetime - stepTime);

                if (projectile.areaActive)
                {
                    projectile.areaAge      += stepTime;
                    projectile.areaTickTime += stepTime;

                    const bool  damageTick = projectile.areaTickTime >= 1.0f || projectile.lifetime <= 0.0f;
                    const float tickDamage = projectile.damage * projectile.areaTickTime / projectile.areaDuration;

                    projectile.radius = projectile.areaRadius * std::clamp(projectile.areaAge / projectile.areaGrowthTime, 0.0f, 1.0f);

                    if (isPlayerProjectile && damageTick)
                    {
                        // The listed damage is the total exposure damage over the area's lifetime.
                        _rEnemyManager.ApplyPoisonDamage(projectile, tickDamage);
                    }

                    else if (!isPlayerProjectile)
                    {
                        const Engine::Math::cVec3f offset = _rPlayerPosition - projectile.position;

                        const float hitRadius   = projectile.radius + 0.4f;
                        const bool  insideArea  = projectile.type == eProjectileType::EnemySpore
                            ? projectile.ContainsGroundPoint(_rPlayerPosition)
                            : offset.x() * offset.x() + offset.z() * offset.z() <= hitRadius * hitRadius && offset.y() <= 0.6f && offset.y() >= -1.8f;

                        if (insideArea)
                        {
                            if (projectile.type == eProjectileType::EnemyShockwave)
                            {
                                if (!projectile.hitPlayer)
                                {
                                    m_pendingPlayerDamage += projectile.damage;
                                    projectile.hitPlayer = true;
                                }
                            }
                            else if (damageTick)
                            {
                                m_pendingPlayerDamage += tickDamage;
                            }
                        }
                    }
                    if (damageTick)
                        projectile.areaTickTime = 0.0f;
                    continue;
                }

                if (isPlayerProjectile && projectile.type != eProjectileType::PlayerReflected
                    && !projectile.isAreaOfEffect && _rEnemyManager.TryReflectPlayerProjectile(projectile, _rPlayerPosition))
                    isPlayerProjectile = false;

                if (MoveProjectile(projectile, stepTime))
                {
                    if (projectile.isAreaOfEffect)
                        activateArea(projectile);
                    else
                    {
                        m_impactEvents.push_back({ projectile.id, projectile.position, projectile.type });
                        projectile.lifetime = 0.0f;
                    }
                    continue;
                }

                if (isPlayerProjectile)
                {
                    // Area shots detonate on contact; their damage comes from the expanding area.
                    const float hitRadius = projectile.isAreaOfEffect ? 0.25f : projectile.radius;
                    const std::span<const sEnemyHandle> hitEnemies(projectile.hitEnemies.data(), projectile.hitEnemyCount);
                    const sEnemyHandle hitEnemy = _rEnemyManager.ApplyDamageAtIgnoring(projectile.position, hitRadius,
                        projectile.isAreaOfEffect ? 0.0f : projectile.damage, hitEnemies);

                    if (hitEnemy.IsValid())
                    {
                        if (projectile.isAreaOfEffect)
                        {
                            activateArea(projectile);
                        }
                        else
                        {
                            m_impactEvents.push_back({ projectile.id, projectile.position, projectile.type });
                            projectile.hitEnemies[projectile.hitEnemyCount++] = hitEnemy;
                            if (projectile.piercesRemaining == 0 || projectile.hitEnemyCount == sProjectile::c_maxHitEnemies)
                                projectile.lifetime = 0.0f;
                            else
                                --projectile.piercesRemaining;
                        }
                    }
                }
                else
                {
                    const float playerHitRadius = projectile.type == eProjectileType::EnemyCone
                        ? projectile.radius + 0.4f
                        : 0.6f;

                    if (Engine::Math::cVec3f::distanceSquared(projectile.position, playerCenter) <= playerHitRadius * playerHitRadius)
                    {
                        if (projectile.isAreaOfEffect)
                            activateArea(projectile);
                        else
                        {
                            m_pendingPlayerDamage += projectile.damage;
                            m_impactEvents.push_back({ projectile.id, projectile.position, projectile.type });
                            projectile.lifetime = 0.0f;
                        }
                    }
                }

                if (projectile.isAreaOfEffect && !projectile.areaActive && projectile.lifetime <= 0.0f)
                    activateArea(projectile);
            }
        }

        std::erase_if(m_projectiles,
                      [](const sProjectile& _rProjectile)
                      {
                          return _rProjectile.lifetime <= 0.0f;
                      });
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cProjectileManager::Clear()
    {
        m_projectiles.clear();
        m_impactEvents.clear();
        m_pendingPlayerDamage = 0.0f;
        m_nextId              = 1;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const std::vector<sProjectile>& cProjectileManager::GetProjectiles() const
    {
        return m_projectiles;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cProjectileManager::ConsumePlayerDamage()
    {
        const float damage    = m_pendingPlayerDamage;
        m_pendingPlayerDamage = 0.0f;
        return damage;
    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------
