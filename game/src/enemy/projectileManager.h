#pragma once

#include "math/vector3.h"

#include <cstdint>
#include <vector>

namespace Gameplay
{
    class cEnemyManager;

    enum class eProjectileType
    {
        EnemyCone,
        EnemySpore,
        PlayerSphere
    };

    struct sProjectile
    {
        uint64_t id = 0;
        Engine::Math::cVec3f position;
        Engine::Math::cVec3f direction;
        float speed          = 0.0f;
        float damage         = 0.0f;
        float lifetime       = 0.0f;
        eProjectileType type = eProjectileType::EnemyCone;
    };

    struct sProjectileSpawnDesc
    {
        Engine::Math::cVec3f position;
        Engine::Math::cVec3f direction;
        float speed;
        float damage;
        float lifetime;
    };

    class cProjectileManager
    {
        public:
            uint64_t SpawnCone(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnSpore(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnPlayerSphere(const sProjectileSpawnDesc& _rDesc);

            void Update(float _deltaTime, const Engine::Math::cVec3f& _rPlayerPosition, cEnemyManager& _rEnemyManager);
            void Clear();

            const std::vector<sProjectile>& GetProjectiles() const;
            float ConsumePlayerDamage();

        private:

            uint64_t Spawn(const sProjectileSpawnDesc& _rDesc, eProjectileType _type);

        private:

            std::vector<sProjectile> m_projectiles;
            uint64_t m_nextId           = 1;
            float m_pendingPlayerDamage = 0.0f;
    };
}
