#pragma once

#include "math/vector3.h"
#include "../world/enemy/enemySpawn.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace Gameplay
{
    class cProjectileManager;

    struct sEnemyHandle
    {
        uint32_t index      = std::numeric_limits<uint32_t>::max();
        uint32_t generation = 0;

        bool operator==(const sEnemyHandle&) const = default;
        bool IsValid() const
        {
            return index != std::numeric_limits<uint32_t>::max();
        }
    };

    enum class eEnemyState
    {
        Idle,
        Chase,
        AttackWindup,
        AttackRecovery,
        Dead
    };

    enum class eEnemyAttackType
    {
        Melee,
        ConeProjectile
    };

    struct sEnemyDefinition
    {
        float maxHealth;
        float movementSpeed;
        float aggroRange;
        float attackRange;
        float preferredRange;
        float attackDamage;
        float attackCooldown;
        float attackWindup;
        float attackRecovery;
        eEnemyAttackType attackType;
    };

    struct sEnemy
    {
        sEnemyHandle handle;
        World::sEnemyType::Enum type = World::sEnemyType::Undefined;
        Engine::Math::cVec3f position;
        Engine::Math::cVec3f attackDirection;
        float rotation             = 0.0f;
        float health               = 0.0f;
        float stateTime            = 0.0f;
        float attackCooldown       = 0.0f;
        float attackPoseWeight     = 0.0f;
        eEnemyState state          = eEnemyState::Idle;
        uint64_t transformRevision = 1;
    };

    struct sEnemyUpdateContext
    {
        float deltaTime;
        Engine::Math::cVec3f playerPosition;
    };

    class cEnemyManager
    {
        public:

            sEnemyHandle Spawn(World::sEnemyType::Enum _type, const Engine::Math::cVec3f& _rPosition, float _rotation);
            void Update(const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager);
            void Clear();

            void ApplyDamage(sEnemyHandle _handle, float _damage);
            bool ApplyDamageAt(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage);
            const sEnemy* TryGetEnemy(sEnemyHandle _handle) const;
            float ConsumePlayerDamage();
            float GetMaxHealth(World::sEnemyType::Enum _type) const;

        private:

            struct sEnemySlot
            {
                sEnemy enemy;
                uint32_t generation = 1;
                bool occupied       = false;
            };

        private:

            const sEnemyDefinition& GetDefinition(World::sEnemyType::Enum _type) const;
            void UpdateEnemy(sEnemy& _rEnemy, const sEnemyDefinition& _rDefinition, const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager);
            void BeginAttack(sEnemy& _rEnemy, const Engine::Math::cVec3f& _rDirection);
            void ExecuteAttack(sEnemy& _rEnemy, const sEnemyDefinition& _rDefinition, const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager);

        private:

            std::vector<sEnemySlot> m_slots;
            std::vector<uint32_t> m_freeSlots;
            float m_pendingPlayerDamage = 0.0f;
    };
}
