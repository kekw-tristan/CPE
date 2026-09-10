#pragma once

#include "math/vector3.h"
#include "../world/enemy/enemySpawn.h"

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace Gameplay
{
    class cProjectileManager;
    struct sProjectile;

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
        Dash,
        Dead
    };

    enum class eEnemyAttackType
    {
        Melee,
        ConeProjectile,
        SporeProjectile,
        Shockwave,
        Dash
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
        float guardedDamageMultiplier = 1.0f;
        float dashSpeed = 0.0f;
        float dashDuration = 0.0f;
    };

    struct sEnemy
    {
        sEnemyHandle handle;
        World::sEnemyType::Enum type = World::sEnemyType::Undefined;
        World::sBossId::Enum bossId = World::sBossId::Undefined;
        Engine::Math::cVec3f position;
        Engine::Math::cVec3f attackDirection;
        bool isBoss = false;
        float scale = 1.0f;
        Engine::Math::cVec3f homePosition;
        sEnemyDefinition definition{};
        float rotation             = 0.0f;
        float health               = 0.0f;
        float stateTime            = 0.0f;
        float attackCooldown       = 0.0f;
        float attackPoseWeight     = 0.0f;
        bool dashHitPlayer         = false;
        eEnemyState state          = eEnemyState::Idle;
        uint64_t transformRevision = 1;
    };

    struct sEnemyDeathEvent
    {
        sEnemyHandle handle;
        Engine::Math::cVec3f position;
        bool isBoss = false;
        World::sBossId::Enum bossId = World::sBossId::Undefined;
    };

    struct sEnemyUpdateContext
    {
        float deltaTime;
        Engine::Math::cVec3f playerPosition;
    };

    class cEnemyManager
    {
        public:

            sEnemyHandle Spawn(World::sEnemyType::Enum _type, const Engine::Math::cVec3f& _rPosition, float _rotation, bool _isBoss = false, World::sBossId::Enum _bossId = World::sBossId::Undefined);
            void Update(const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager);
            void Clear();
            void SetActive(sEnemyHandle _handle, bool _active);

            void ApplyDamage(sEnemyHandle _handle, float _damage);
            bool ApplyDamageAt(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage);
            sEnemyHandle ApplyDamageAtIgnoring(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage, std::span<const sEnemyHandle> _rIgnoredHandles);
            bool ApplyDamageInRadius(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage);
            const sEnemy* TryGetEnemy(sEnemyHandle _handle) const;
            float FindAimDistance(const Engine::Math::cVec3f& _rOrigin, const Engine::Math::cVec3f& _rDirection, float _maximumDistance) const;
            void ApplyPoisonDamage(const sProjectile& _rArea, float _damage);
            float ConsumePlayerDamage();
            const std::vector<sEnemyDeathEvent>& GetDeathEvents() const;
            void ClearDeathEvents();
            float GetMaxHealth(World::sEnemyType::Enum _type) const;

        private:

            struct sEnemySlot
            {
                sEnemy enemy;
                uint32_t generation = 1;
                bool occupied       = false;
                bool active         = true;
            };

        private:

            const sEnemyDefinition& GetDefinition(World::sEnemyType::Enum _type) const;
            void UpdateEnemy(sEnemy& _rEnemy, const sEnemyDefinition& _rDefinition, const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager);
            void MoveEnemy(sEnemy& _rEnemy, const Engine::Math::cVec3f& _rMovement);
            void BeginAttack(sEnemy& _rEnemy, const Engine::Math::cVec3f& _rDirection);
            void ExecuteAttack(sEnemy& _rEnemy, const sEnemyDefinition& _rDefinition, const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager);

        private:

            std::vector<sEnemySlot> m_slots;
            std::vector<uint32_t> m_freeSlots;
            std::vector<uint32_t> m_activeSlots;

            std::vector<sEnemyDeathEvent> m_deathEvents;

            float m_pendingPlayerDamage = 0.0f;
    };
}
