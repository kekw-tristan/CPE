#pragma once

#include "enemyManager.h"
#include "math/vector3.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Gameplay
{
    enum class eProjectileType
    {
        EnemyCone,
        EnemySpore,
        EnemyShockwave,
        PlayerSphere,
        PlayerCone,
        PlayerSpore
    };

    struct sProjectile
    {
        static constexpr size_t c_maxHitEnemies = 5;

        static constexpr size_t c_maxGroundSamples = 81;

        float GetGroundSampleRadius(size_t _index) const;
        bool ContainsGroundPoint(const Engine::Math::cVec3f& _rPosition) const;

        std::array<Engine::Math::cVec3f, c_maxGroundSamples> groundSamples{};
        std::array<Engine::Math::cVec3f, c_maxGroundSamples> groundNormals{};

        size_t groundSampleCount = 0;
        float groundSampleRadius = 0.0f;
        
        uint64_t id = 0;
        
        Engine::Math::cVec3f position;
        Engine::Math::cVec3f direction;

        float gravity           = 0.0f;
        float flightAge         = 0.0f;
        float speed             = 0.0f;
        float damage            = 0.0f;
        float lifetime          = 0.0f;
        float radius            = 0.8f;
        float visualScale       = 1.0f;

        bool  isAreaOfEffect    = false;
        bool  areaActive        = false;
        bool  hitPlayer         = false;
        bool  channeling        = false;
        
        float areaAge           = 0.0f;
        float areaTickTime      = 0.0f;
        float areaRadius        = 0.0f;
        float areaDuration      = 1.0f;
        float areaGrowthTime    = 0.5f;
        int   piercesRemaining  = 0;

        std::array<sEnemyHandle, c_maxHitEnemies> hitEnemies{};
        uint8_t hitEnemyCount = 0;
        eProjectileType type = eProjectileType::EnemyCone;
    };

    struct sProjectileImpactEvent
    {
        uint64_t id = 0;
        Engine::Math::cVec3f position{};
        eProjectileType type = eProjectileType::EnemySpore;
    };

    struct sProjectileSpawnDesc
    {
        Engine::Math::cVec3f position{};
        Engine::Math::cVec3f direction{};

        float gravity   = 0.0f;
        float speed     = 0.0f;
        float damage    = 0.0f;
        float lifetime  = 0.0f;
        float radius    = 0.8f;
        float visualScale = 1.0f;
        
        bool isAreaOfEffect = false;
        
        float areaRadius     = 0.0f;
        float areaDuration   = 1.0f;
        float areaGrowthTime = 0.5f;
        
        int pierces = 0;
    };

    void AimMushroomThrow(sProjectileSpawnDesc& _rDesc, const Engine::Math::cVec3f& _rTarget);

    class cProjectileManager
    {
        public:

            uint64_t SpawnCone(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnSpore(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnShockwave(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnPlayerSphere(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnPlayerChannelCone(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnPlayerCone(const sProjectileSpawnDesc& _rDesc);
            uint64_t SpawnPlayerSpore(const sProjectileSpawnDesc& _rDesc);

            bool UpdatePlayerChannelCone(uint64_t _id, const Engine::Math::cVec3f& _rPosition,
                                         const Engine::Math::cVec3f& _rDirection, float _radius, float _visualScale);
            bool ReleasePlayerChannelCone(uint64_t _id, const Engine::Math::cVec3f& _rDirection,
                                          float _speed, float _damage, float _lifetime);

            void Update(float _deltaTime, const Engine::Math::cVec3f& _rPlayerPosition, cEnemyManager& _rEnemyManager);
            void Clear();

            const std::vector<sProjectile>& GetProjectiles() const;
            float ConsumePlayerDamage();
            const std::vector<sProjectileImpactEvent>& GetImpactEvents() const { return m_impactEvents; }
            void ClearImpactEvents() { m_impactEvents.clear(); }

        private:

            uint64_t Spawn(const sProjectileSpawnDesc& _rDesc, eProjectileType _type);

        private:

            std::vector<sProjectile> m_projectiles;
            std::vector<sProjectileImpactEvent> m_impactEvents;

            uint64_t m_nextId           = 1;
            float m_pendingPlayerDamage = 0.0f;
    };
}
