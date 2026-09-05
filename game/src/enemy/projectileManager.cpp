#include "projectileManager.h"

#include "enemyManager.h"

#include <algorithm>

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{
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

    uint64_t cProjectileManager::SpawnPlayerSphere(const sProjectileSpawnDesc& _rDesc)
    {
        return Spawn(_rDesc, eProjectileType::PlayerSphere);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint64_t cProjectileManager::Spawn(const sProjectileSpawnDesc& _rDesc, eProjectileType _type)
    {
        sProjectile projectile{};

        projectile.id        = m_nextId++;
        projectile.position  = _rDesc.position;
        projectile.direction = _rDesc.direction.normalized();
        projectile.speed     = _rDesc.speed;
        projectile.damage    = _rDesc.damage;
        projectile.lifetime  = _rDesc.lifetime;
        projectile.type      = _type;

        m_projectiles.push_back(projectile);

        return projectile.id;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cProjectileManager::Update(float _deltaTime, const Engine::Math::cVec3f& _rPlayerPosition, cEnemyManager& _rEnemyManager)
    {
        constexpr float c_hitRadius  = 0.6f;
        const float hitRadiusSquared = c_hitRadius * c_hitRadius;

        for (sProjectile& projectile : m_projectiles)
        {
            projectile.position += projectile.direction * (projectile.speed * _deltaTime);
            projectile.lifetime -= _deltaTime;

            if (projectile.type == eProjectileType::PlayerSphere)
            {
                if (_rEnemyManager.ApplyDamageAt(projectile.position, 0.8f, projectile.damage))
                    projectile.lifetime = 0.0f;
            }
            else
            {
                const Engine::Math::cVec3f playerCenter = _rPlayerPosition + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f);
                if (Engine::Math::cVec3f::distanceSquared(projectile.position, playerCenter) <= hitRadiusSquared)
                {
                    m_pendingPlayerDamage += projectile.damage;
                    projectile.lifetime = 0.0f;
                }
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
