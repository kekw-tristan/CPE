#include "enemyManager.h"

#include "projectileManager.h"

#include "physics/collider.h"
#include "physics/collisionWorld.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        constexpr sEnemyDefinition c_crawlerDefinition
        {
            45.0f, 2.8f, 14.0f, 11.0f, 8.0f,
            10.0f, 1.8f, 0.35f, 0.3f,
            eEnemyAttackType::ConeProjectile
        };

        constexpr sEnemyDefinition c_thornwolfDefinition
        {
            65.0f, 4.2f, 16.0f, 1.6f, 1.2f,
            12.0f, 0.9f, 0.25f, 0.25f,
            eEnemyAttackType::Melee
        };

        constexpr sEnemyDefinition c_sporecapDefinition
        {
            80.0f, 1.5f, 15.0f, 12.0f, 9.0f,
            16.0f, 2.4f, 0.65f, 0.5f,
            eEnemyAttackType::ConeProjectile
        };

        constexpr sEnemyDefinition c_bruteDefinition
        {
            120.0f, 2.2f, 11.0f, 1.7f, 1.4f,
            22.0f, 1.4f, 0.45f, 0.45f,
            eEnemyAttackType::Melee
        };

        // -------------------------------------------------------------------------------------------------------------------------

        float HorizontalDistance(const Engine::Math::cVec3f& _rFirst, const Engine::Math::cVec3f& _rSecond)
        {
            const float x = _rSecond.x() - _rFirst.x();
            const float z = _rSecond.z() - _rFirst.z();
            return std::sqrt(x * x + z * z);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Engine::Math::cVec3f HorizontalDirection(const Engine::Math::cVec3f& _rFrom, const Engine::Math::cVec3f& _rTo)
        {
            Engine::Math::cVec3f direction(_rTo.x() - _rFrom.x(), 0.0f, _rTo.z() - _rFrom.z());
            direction.normalize();
            return direction;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cEnemyManager::GetMaxHealth(World::sEnemyType::Enum _type) const
    {
        return GetDefinition(_type).maxHealth;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sEnemyHandle cEnemyManager::Spawn(World::sEnemyType::Enum _type, const Engine::Math::cVec3f& _rPosition, float _rotation, bool _isBoss)
    {
        const sEnemyDefinition& definition = GetDefinition(_type);
        uint32_t slotIndex;

        if (m_freeSlots.empty())
        {
            slotIndex = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }
        else
        {
            slotIndex = m_freeSlots.back();
            m_freeSlots.pop_back();
        }

        sEnemySlot& slot    = m_slots[slotIndex];
        slot.occupied       = true;
        slot.active         = true;
        m_activeSlots.push_back(slotIndex);

        slot.enemy          = {};
        slot.enemy.handle   = {slotIndex, slot.generation};
        slot.enemy.type     = _type;
        slot.enemy.position = _rPosition;
        slot.enemy.rotation = _rotation;
        slot.enemy.isBoss = _isBoss;
        slot.enemy.scale = _isBoss ? 2.5f : 1.0f;
        slot.enemy.homePosition = _rPosition;
        slot.enemy.definition = definition;
        if (_isBoss)
        {
            slot.enemy.definition.maxHealth *= 6.0f;
            slot.enemy.definition.attackDamage *= 1.5f;
            slot.enemy.definition.aggroRange = 24.0f;
            slot.enemy.definition.attackWindup *= 1.4f;
            if (definition.attackType == eEnemyAttackType::Melee)
                slot.enemy.definition.attackRange *= 2.5f;
        }
        slot.enemy.health = slot.enemy.definition.maxHealth;

        return slot.enemy.handle;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::Update(const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager)
    {
        for (uint32_t slotIndex : m_activeSlots)
        {
            sEnemySlot& slot = m_slots[slotIndex];

            if (!slot.occupied || !slot.active || slot.enemy.state == eEnemyState::Dead)
                continue;

            const Engine::Math::cVec3f previousPosition = slot.enemy.position;
            const float previousRotation                = slot.enemy.rotation;
            UpdateEnemy(slot.enemy, slot.enemy.definition, _rContext, _rProjectileManager);

            if (slot.enemy.position != previousPosition || slot.enemy.rotation != previousRotation)
                ++slot.enemy.transformRevision;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::SetActive(sEnemyHandle _handle, bool _active)
    {
        if (_handle.index < m_slots.size())
        {
            auto& slot = m_slots[_handle.index];

            if (slot.occupied && slot.generation == _handle.generation && slot.active != _active)
            {
                slot.active = _active;

                if (_active)
                    m_activeSlots.push_back(_handle.index);
                else
                    std::erase(m_activeSlots, _handle.index);
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::Clear()
    {
        m_slots.clear();
        m_freeSlots.clear();
        m_activeSlots.clear();

        m_pendingPlayerDamage = 0.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::ApplyDamage(sEnemyHandle _handle, float _damage)
    {
        if (_damage <= 0.0f || _handle.index >= m_slots.size())
            return;

        sEnemySlot& slot = m_slots[_handle.index];
        if (!slot.occupied || slot.generation != _handle.generation || slot.enemy.state == eEnemyState::Dead)
            return;

        slot.enemy.health = std::max(0.0f, slot.enemy.health - _damage);
        if (slot.enemy.health == 0.0f)
        {
            slot.enemy.state     = eEnemyState::Dead;
            slot.enemy.stateTime = 0.0f;
            ++slot.enemy.transformRevision;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cEnemyManager::ApplyDamageAt(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage)
    {


        for (uint32_t slotIndex : m_activeSlots)
        {
            sEnemySlot& slot = m_slots[slotIndex];

            if (!slot.occupied || !slot.active || slot.enemy.state == eEnemyState::Dead)
                continue;

            const float hitRadius = _radius + (slot.enemy.scale - 1.0f) * 0.65f;
            const float radiusSquared = hitRadius * hitRadius;
            const Engine::Math::cVec3f enemyCenter = slot.enemy.position + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f);
            if (Engine::Math::cVec3f::distanceSquared(_rPosition, enemyCenter) > radiusSquared)
                continue;

            ApplyDamage(slot.enemy.handle, _damage);
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sEnemy* cEnemyManager::TryGetEnemy(sEnemyHandle _handle) const
    {
        if (_handle.index >= m_slots.size())
            return nullptr;

        const sEnemySlot& slot = m_slots[_handle.index];
        if (!slot.occupied || slot.generation != _handle.generation)
            return nullptr;

        return &slot.enemy;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cEnemyManager::ConsumePlayerDamage()
    {
        const float damage    = m_pendingPlayerDamage;
        m_pendingPlayerDamage = 0.0f;
        return damage;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sEnemyDefinition& cEnemyManager::GetDefinition(World::sEnemyType::Enum _type) const
    {
        switch (_type)
        {
            case World::sEnemyType::ForestCrawler:
                return c_crawlerDefinition;
            case World::sEnemyType::ForestBrute:
                return c_bruteDefinition;
            case World::sEnemyType::ForestThornwolf:
                return c_thornwolfDefinition;
            case World::sEnemyType::ForestSporecap:
                return c_sporecapDefinition;
            default:
                throw std::invalid_argument("No definition exists for this enemy type.");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::UpdateEnemy(sEnemy& _rEnemy, const sEnemyDefinition& _rDefinition, const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager)
    {
        if (_rEnemy.isBoss && (std::abs(_rContext.playerPosition.x() - _rEnemy.homePosition.x()) > 12.5f
            || std::abs(_rContext.playerPosition.z() - _rEnemy.homePosition.z()) > 12.5f))
        {
            _rEnemy.position = _rEnemy.homePosition;
            _rEnemy.health = _rDefinition.maxHealth;
            _rEnemy.state = eEnemyState::Idle;
            _rEnemy.stateTime = 0.0f;
            _rEnemy.attackPoseWeight = 0.0f;
            _rEnemy.attackCooldown = 0.0f;
            return;
        }
        _rEnemy.stateTime += _rContext.deltaTime;
        _rEnemy.attackCooldown = std::max(0.0f, _rEnemy.attackCooldown - _rContext.deltaTime);

        if (_rEnemy.state == eEnemyState::AttackWindup)
            _rEnemy.attackPoseWeight = std::clamp(_rEnemy.stateTime / _rDefinition.attackWindup, 0.0f, 1.0f);

        else if (_rEnemy.state == eEnemyState::AttackRecovery)
            _rEnemy.attackPoseWeight = 1.0f - std::clamp(_rEnemy.stateTime / _rDefinition.attackRecovery, 0.0f, 1.0f);

        else
            _rEnemy.attackPoseWeight = 0.0f;

        constexpr float c_idleUpdateInterval = 0.2f;
        if (_rEnemy.state == eEnemyState::Idle && _rEnemy.stateTime < c_idleUpdateInterval)
            return;

        const float distance                = HorizontalDistance(_rEnemy.position, _rContext.playerPosition);
        const Engine::Math::cVec3f toPlayer = HorizontalDirection(_rEnemy.position, _rContext.playerPosition);

        if (_rEnemy.state == eEnemyState::Idle)
        {
            if (distance <= _rDefinition.aggroRange)
            {
                _rEnemy.state     = eEnemyState::Chase;
                _rEnemy.stateTime = 0.0f;
            }
            else
            {
                _rEnemy.stateTime = 0.0f;
            }
            return;
        }

        if (!toPlayer.isZero())
            _rEnemy.rotation = std::atan2(toPlayer.x(), toPlayer.z());

        if (_rEnemy.state == eEnemyState::AttackWindup)
        {
            if (_rEnemy.stateTime >= _rDefinition.attackWindup)
            {
                ExecuteAttack(_rEnemy, _rDefinition, _rContext, _rProjectileManager);
                _rEnemy.state          = eEnemyState::AttackRecovery;
                _rEnemy.stateTime      = 0.0f;
                _rEnemy.attackCooldown = _rDefinition.attackCooldown;
            }
            return;
        }

        if (_rEnemy.state == eEnemyState::AttackRecovery)
        {
            if (_rEnemy.stateTime >= _rDefinition.attackRecovery)
            {
                _rEnemy.state     = eEnemyState::Chase;
                _rEnemy.stateTime = 0.0f;
            }
            return;
        }

        if (distance > _rDefinition.aggroRange * 1.5f)
        {
            _rEnemy.state     = eEnemyState::Idle;
            _rEnemy.stateTime = 0.0f;
            return;
        }

        const bool hasUsefulAttackDistance = _rDefinition.attackType == eEnemyAttackType::Melee || distance >= _rDefinition.preferredRange - 1.0f;

        if (distance <= _rDefinition.attackRange && hasUsefulAttackDistance && _rEnemy.attackCooldown <= 0.0f)
        {
            BeginAttack(_rEnemy, toPlayer);
            return;
        }

        Engine::Math::cVec3f movementDirection;
        if (_rDefinition.attackType == eEnemyAttackType::Melee || distance > _rDefinition.preferredRange + 1.0f)
            movementDirection = toPlayer;
        else if (distance < _rDefinition.preferredRange - 1.0f)
            movementDirection = -toPlayer;

        if (movementDirection.isZero())
            return;

        const float c_radius = 0.45f * _rEnemy.scale;
        constexpr float c_halfHeight = 0.75f;
        Engine::Physics::sCapsuleCollider collider{};
        collider.center     = _rEnemy.position + Engine::Math::cVec3f(0.0f, c_radius + c_halfHeight, 0.0f);
        collider.radius     = c_radius;
        collider.halfHeight = c_halfHeight;

        const Engine::Math::cVec3f movement = movementDirection * (_rDefinition.movementSpeed * _rContext.deltaTime);
        const Engine::Math::cVec3f center   = Engine::Physics::CollisionWorld::MoveCapsule(collider, movement);
        _rEnemy.position = {center.x(), _rEnemy.position.y(), center.z()};
        if (_rEnemy.isBoss)
        {
            _rEnemy.position = {
                std::clamp(_rEnemy.position.x(), _rEnemy.homePosition.x() - 10.0f, _rEnemy.homePosition.x() + 10.0f),
                _rEnemy.position.y(),
                std::clamp(_rEnemy.position.z(), _rEnemy.homePosition.z() - 10.0f, _rEnemy.homePosition.z() + 10.0f)
            };
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::BeginAttack(sEnemy& _rEnemy, const Engine::Math::cVec3f& _rDirection)
    {
        _rEnemy.attackDirection = _rDirection;
        _rEnemy.state           = eEnemyState::AttackWindup;
        _rEnemy.stateTime       = 0.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::ExecuteAttack(sEnemy& _rEnemy, const sEnemyDefinition& _rDefinition, const sEnemyUpdateContext& _rContext, cProjectileManager& _rProjectileManager)
    {
        if (_rDefinition.attackType == eEnemyAttackType::Melee)
        {
            if (HorizontalDistance(_rEnemy.position, _rContext.playerPosition) <= _rDefinition.attackRange + 0.25f)
                m_pendingPlayerDamage += _rDefinition.attackDamage;
            return;
        }

        sProjectileSpawnDesc projectile{};

        projectile.position  = _rEnemy.position + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f) + _rEnemy.attackDirection * 0.7f;
        projectile.direction = _rEnemy.attackDirection;
        projectile.speed     = 9.0f;
        projectile.damage    = _rDefinition.attackDamage;
        projectile.lifetime  = 2.5f;

        if (_rEnemy.type == World::sEnemyType::ForestSporecap)
            _rProjectileManager.SpawnSpore(projectile);
        else
            _rProjectileManager.SpawnCone(projectile);
    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------
