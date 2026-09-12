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
            65.0f, 4.2f, 16.0f, 6.5f, 1.2f,
            16.0f, 2.2f, 0.65f, 0.65f,
            eEnemyAttackType::Dash, 1.0f, 14.0f, 0.45f
        };

        constexpr sEnemyDefinition c_sporecapDefinition
        {
            80.0f, 1.5f, 15.0f, 12.0f, 9.0f,
            16.0f, 2.4f, 0.65f, 0.5f,
            eEnemyAttackType::SporeProjectile
        };

        constexpr sEnemyDefinition c_bruteDefinition
        {
            120.0f, 2.2f, 11.0f, 4.5f, 1.4f,
            22.0f, 2.6f, 0.9f, 0.8f,
            eEnemyAttackType::Shockwave
        };

        // -------------------------------------------------------------------------------------------------------------------------

        constexpr sEnemyDefinition c_thornshooterDefinition
        {
            55.0f, 2.6f, 18.0f, 15.0f, 10.0f,
            14.0f, 1.7f, 0.6f, 0.4f,
            eEnemyAttackType::ConeProjectile
        };

        constexpr sEnemyDefinition c_rootchargerDefinition
        {
            90.0f, 3.0f, 17.0f, 8.0f, 1.2f,
            24.0f, 3.2f, 0.85f, 0.9f,
            eEnemyAttackType::Dash, 1.0f, 16.0f, 0.55f
        };

        constexpr sEnemyDefinition c_barkguardDefinition
        {
            200.0f, 1.5f, 13.0f, 1.9f, 1.5f,
            26.0f, 2.0f, 0.8f, 1.0f,
            eEnemyAttackType::Melee, 0.3f
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

        // -------------------------------------------------------------------------------------------------------------------------

        float RaySphereDistance(const Engine::Math::cVec3f& _rOrigin, const Engine::Math::cVec3f& _rDirection, const Engine::Math::cVec3f& _rCenter, float _radius, float _maximumDistance)
        {
            const Engine::Math::cVec3f offset = _rOrigin - _rCenter;
            
            const float b = offset.dot(_rDirection);
            const float c = offset.dot(offset) - _radius * _radius;

            const float discriminant = b * b - c;
        
            if (discriminant < 0.0f)
                return _maximumDistance;
        
            const float root            = std::sqrt(discriminant);
            const float nearDistance    = -b - root;
            const float farDistance     = -b + root;
        
            if (nearDistance >= 0.0f && nearDistance < _maximumDistance)
                return nearDistance;
        
            if (farDistance >= 0.0f && farDistance < _maximumDistance)
                return farDistance;
        
            return _maximumDistance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float RayEnemyCapsuleDistance(const Engine::Math::cVec3f& _rOrigin, const Engine::Math::cVec3f& _rDirection, const sEnemy& _rEnemy, float _maximumDistance)
        {
            const float radius      = 0.45f * _rEnemy.scale;
            const float halfHeight  = 0.75f * _rEnemy.scale;
        
            const Engine::Math::cVec3f center = _rEnemy.position + Engine::Math::cVec3f(0.0f, radius + halfHeight, 0.0f);

            const float bottomY = center.y() - halfHeight;
            const float topY    = center.y() + halfHeight;
        
            float nearestDistance = _maximumDistance;
        
            const float originX     = _rOrigin.x() - center.x();
            const float originZ     = _rOrigin.z() - center.z();
            const float directionX  = _rDirection.x();
            const float directionZ  = _rDirection.z();
        
            const float a = directionX * directionX + directionZ * directionZ;
        
            if (a > 0.000001f)
            {
                const float b = 2.0f * (originX * directionX + originZ * directionZ);
                const float c = originX * originX + originZ * originZ - radius * radius;
                const float discriminant = b * b - 4.0f * a * c;
            
                if (discriminant >= 0.0f)
                {
                    const float root = std::sqrt(discriminant);
                    const float inverseDenominator = 1.0f / (2.0f * a);
                    const float distances[2] = { (-b - root) * inverseDenominator, (-b + root) * inverseDenominator };
                
                    for (float distance : distances)
                    {
                        if (distance < 0.0f || distance >= nearestDistance)
                            continue;
                    
                        const float hitY = _rOrigin.y() + _rDirection.y() * distance;
                    
                        if (hitY >= bottomY && hitY <= topY)
                            nearestDistance = distance;
                    }
                }
            }
        
            nearestDistance = RaySphereDistance(_rOrigin, _rDirection, Engine::Math::cVec3f(center.x(), bottomY, center.z()), radius, nearestDistance);
            nearestDistance = RaySphereDistance(_rOrigin, _rDirection, Engine::Math::cVec3f(center.x(), topY, center.z()), radius, nearestDistance);
        
            return nearestDistance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool SphereIntersectsEnemyCapsule(const Engine::Math::cVec3f& _rPosition, float _radius, const sEnemy& _rEnemy)
        {
            const float capsuleRadius       = 0.45f * _rEnemy.scale;
            const float capsuleHalfHeight   = 0.75f * _rEnemy.scale;
        
            const Engine::Math::cVec3f capsuleCenter = _rEnemy.position + Engine::Math::cVec3f(0.0f, capsuleRadius + capsuleHalfHeight, 0.0f);
            const Engine::Math::cVec3f capsuleBottom = capsuleCenter - Engine::Math::cVec3f(0.0f, capsuleHalfHeight, 0.0f);
            const Engine::Math::cVec3f capsuleTop    = capsuleCenter + Engine::Math::cVec3f(0.0f, capsuleHalfHeight, 0.0f);
        
            const Engine::Math::cVec3f capsuleSegment = capsuleTop - capsuleBottom;

            const float segmentLengthSquared = capsuleSegment.lengthSquared();
            
            const float t = segmentLengthSquared > 0.000001f ? std::clamp((_rPosition - capsuleBottom).dot(capsuleSegment) / segmentLengthSquared, 0.0f, 1.0f) : 0.0f;
            
            const Engine::Math::cVec3f closestPoint = capsuleBottom + capsuleSegment * t;
        
            const float combinedRadius = capsuleRadius + _radius;
        
            return Engine::Math::cVec3f::distanceSquared(_rPosition, closestPoint) <= combinedRadius * combinedRadius;
        }
        
        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cEnemyManager::GetMaxHealth(World::sEnemyType::Enum _type) const
    {
        return GetDefinition(_type).maxHealth;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sEnemyHandle cEnemyManager::Spawn(World::sEnemyType::Enum _type, const Engine::Math::cVec3f& _rPosition, float _rotation, bool _isBoss, World::sBossId::Enum _bossId)
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
        slot.enemy.isBoss   = _isBoss;
        slot.enemy.bossId   = _isBoss ? _bossId : World::sBossId::Undefined;
        
        const bool miniboss = _isBoss && _bossId == World::sBossId::Undefined;

        slot.enemy.scale        = _isBoss ? (miniboss ? 1.8f : 2.5f) : 1.0f;
        slot.enemy.homePosition = _rPosition;
        slot.enemy.definition   = definition;
        if (_isBoss)
        {
            slot.enemy.definition.maxHealth     *= miniboss ? 3.5f : 6.0f;
            slot.enemy.definition.attackDamage  *= miniboss ? 1.25f : 1.5f;
            slot.enemy.definition.aggroRange    = 24.0f;
            slot.enemy.definition.attackWindup  *= 1.4f;

            if (definition.attackType == eEnemyAttackType::Melee)
                slot.enemy.definition.attackRange *= slot.enemy.scale;
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
        m_deathEvents.clear();

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

        const bool  isGuarding  = slot.enemy.state == eEnemyState::Idle || slot.enemy.state == eEnemyState::Chase;
        const float damage      = _damage * (isGuarding ? slot.enemy.definition.guardedDamageMultiplier : 1.0f);

        slot.enemy.health = std::max(0.0f, slot.enemy.health - damage);

        if (slot.enemy.health == 0.0f)
        {
            slot.enemy.state     = eEnemyState::Dead;
            slot.enemy.stateTime = 0.0f;
            ++slot.enemy.transformRevision;
            m_deathEvents.push_back({ slot.enemy.handle, slot.enemy.position, slot.enemy.isBoss, slot.enemy.bossId });
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cEnemyManager::ApplyDamageAt(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage)
    {
        return ApplyDamageAtIgnoring(_rPosition, _radius, _damage, {}).IsValid();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sEnemyHandle cEnemyManager::ApplyDamageAtIgnoring(
        const Engine::Math::cVec3f& _rPosition,
        float _radius,
        float _damage,
        std::span<const sEnemyHandle> _rIgnoredHandles)
    {
        for (uint32_t slotIndex : m_activeSlots)
        {
            sEnemySlot& slot = m_slots[slotIndex];
        
            if (!slot.occupied || !slot.active || slot.enemy.state == eEnemyState::Dead)
                continue;
        
            if (!SphereIntersectsEnemyCapsule(_rPosition, _radius, slot.enemy))
                continue;

            const bool wasAlreadyHit = std::find(_rIgnoredHandles.begin(), _rIgnoredHandles.end(), slot.enemy.handle) != _rIgnoredHandles.end();
            if (wasAlreadyHit)
                continue;
        
            ApplyDamage(slot.enemy.handle, _damage);
            return slot.enemy.handle;
        }
    
        return {};
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cEnemyManager::ApplyDamageInRadius(const Engine::Math::cVec3f& _rPosition, float _radius, float _damage)
    {
        bool hitEnemy = false;

        for (uint32_t slotIndex : m_activeSlots)
        {
            sEnemySlot& slot = m_slots[slotIndex];

            if (!slot.occupied || !slot.active || slot.enemy.state == eEnemyState::Dead)
                continue;

            if (!SphereIntersectsEnemyCapsule(_rPosition, _radius, slot.enemy))
                continue;

            ApplyDamage(slot.enemy.handle, _damage);
            hitEnemy = true;
        }

        return hitEnemy;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::ApplyPoisonDamage(const sProjectile& _rArea, float _damage)
    {
        for (uint32_t slotIndex : m_activeSlots)
        {
            sEnemySlot& slot = m_slots[slotIndex];
            if (slot.occupied && slot.active && slot.enemy.state != eEnemyState::Dead
                && _rArea.ContainsGroundPoint(slot.enemy.position))
            {
                ApplyDamage(slot.enemy.handle, _damage);
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cEnemyManager::FindAimDistance(const Engine::Math::cVec3f& _rOrigin, const Engine::Math::cVec3f& _rDirection, float _maximumDistance) const
    {
        float nearestDistance = _maximumDistance;

        for (uint32_t slotIndex : m_activeSlots)
        {
            const sEnemySlot& slot = m_slots[slotIndex];

            if (!slot.occupied || !slot.active || slot.enemy.state == eEnemyState::Dead)
                continue;

            nearestDistance = RayEnemyCapsuleDistance(_rOrigin, _rDirection, slot.enemy, nearestDistance);
        }

        return nearestDistance;
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

    const std::vector<sEnemyDeathEvent>& cEnemyManager::GetDeathEvents() const
    {
        return m_deathEvents;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::ClearDeathEvents()
    {
        m_deathEvents.clear();
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
            case World::sEnemyType::ForestThornshooter:
                return c_thornshooterDefinition;
            case World::sEnemyType::ForestRootcharger:
                return c_rootchargerDefinition;
            case World::sEnemyType::ForestBarkguard:
                return c_barkguardDefinition;
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

        else if (_rEnemy.state == eEnemyState::Dash)
            _rEnemy.attackPoseWeight = 1.0f;

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

        if (_rEnemy.state == eEnemyState::Dash)
        {
            // Short collision steps also prevent a fast charge from skipping the player.
            float remainingTime = std::min(_rContext.deltaTime,
                std::max(0.0f, _rDefinition.dashDuration - (_rEnemy.stateTime - _rContext.deltaTime)));
            while (remainingTime > 0.000001f)
            {
                const float stepTime = std::min(remainingTime, 0.2f / _rDefinition.dashSpeed);
                const Engine::Math::cVec3f previousPosition = _rEnemy.position;
                MoveEnemy(_rEnemy, _rEnemy.attackDirection * (_rDefinition.dashSpeed * stepTime));
                remainingTime -= stepTime;

                if (!_rEnemy.dashHitPlayer
                    && HorizontalDistance(_rEnemy.position, _rContext.playerPosition) <= 0.45f * _rEnemy.scale + 0.5f
                    && std::abs(_rEnemy.position.y() - _rContext.playerPosition.y()) <= 1.5f)
                {
                    m_pendingPlayerDamage += _rDefinition.attackDamage;
                    _rEnemy.dashHitPlayer = true;
                }

                if (HorizontalDistance(previousPosition, _rEnemy.position) < _rDefinition.dashSpeed * stepTime * 0.5f)
                {
                    _rEnemy.stateTime = _rDefinition.dashDuration;
                    break;
                }
            }

            if (_rEnemy.stateTime >= _rDefinition.dashDuration)
            {
                _rEnemy.state = eEnemyState::AttackRecovery;
                _rEnemy.stateTime = 0.0f;
                _rEnemy.attackCooldown = _rDefinition.attackCooldown;
            }
            return;
        }

        if (!toPlayer.isZero() && !(_rDefinition.attackType == eEnemyAttackType::Dash
            && _rEnemy.state == eEnemyState::AttackWindup))
            _rEnemy.rotation = std::atan2(toPlayer.x(), toPlayer.z());

        if (_rEnemy.state == eEnemyState::AttackWindup)
        {
            if (_rEnemy.stateTime >= _rDefinition.attackWindup)
            {
                if (_rDefinition.attackType == eEnemyAttackType::Dash)
                {
                    _rEnemy.state = eEnemyState::Dash;
                    _rEnemy.stateTime = 0.0f;
                    _rEnemy.dashHitPlayer = false;
                    return;
                }

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

        const bool isRanged = _rDefinition.attackType == eEnemyAttackType::ConeProjectile
            || _rDefinition.attackType == eEnemyAttackType::SporeProjectile;
        const bool hasUsefulAttackDistance = !isRanged || distance >= _rDefinition.preferredRange - 1.0f;

        if (distance <= _rDefinition.attackRange && hasUsefulAttackDistance && _rEnemy.attackCooldown <= 0.0f)
        {
            BeginAttack(_rEnemy, toPlayer);
            return;
        }

        Engine::Math::cVec3f movementDirection;
        if (!isRanged || distance > _rDefinition.preferredRange + 1.0f)
            movementDirection = toPlayer;
        else if (distance < _rDefinition.preferredRange - 1.0f)
            movementDirection = -toPlayer;

        if (movementDirection.isZero())
            return;

        MoveEnemy(_rEnemy, movementDirection * (_rDefinition.movementSpeed * _rContext.deltaTime));
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cEnemyManager::MoveEnemy(sEnemy& _rEnemy, const Engine::Math::cVec3f& _rMovement)
    {
        const float c_radius     = 0.45f * _rEnemy.scale;
        const float c_halfHeight = 0.75f * _rEnemy.scale;

        Engine::Physics::sCapsuleCollider collider{};
        collider.center     = _rEnemy.position + Engine::Math::cVec3f(0.0f, c_radius + c_halfHeight, 0.0f);
        collider.radius     = c_radius;
        collider.halfHeight = c_halfHeight;

        const Engine::Math::cVec3f center = Engine::Physics::CollisionWorld::MoveCapsule(collider, _rMovement);
        _rEnemy.position = {center.x(), _rEnemy.position.y(), center.z()};

        float groundHeight = _rEnemy.position.y();

        if (Engine::Physics::CollisionWorld::FindGroundHeight(_rEnemy.position,
            _rEnemy.position.y() + 1.0f, groundHeight))
        {
            _rEnemy.position = { center.x(), groundHeight, center.z() };
        }

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

        if (_rDefinition.attackType == eEnemyAttackType::Shockwave)
        {
            projectile.position         = _rEnemy.position + Engine::Math::cVec3f(0.0f, 0.2f, 0.0f);
            projectile.damage           = _rDefinition.attackDamage;
            projectile.isAreaOfEffect   = true;
            projectile.areaRadius       = _rDefinition.attackRange + 0.5f;
            projectile.areaDuration     = 0.8f;
            projectile.areaGrowthTime   = 0.8f;
            _rProjectileManager.SpawnShockwave(projectile);
            return;
        }

        projectile.position  = _rEnemy.position + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f) + _rEnemy.attackDirection * 0.7f;
        projectile.direction = _rEnemy.attackDirection;
        projectile.speed     = _rEnemy.type == World::sEnemyType::ForestThornshooter ? 13.0f : 9.0f;
        projectile.damage    = _rDefinition.attackDamage;
        projectile.lifetime  = 2.5f;
        projectile.radius    = _rEnemy.type == World::sEnemyType::ForestThornshooter ? 0.45f : 0.8f;

        if (_rEnemy.type == World::sEnemyType::ForestSporecap)
        {
            const Engine::Math::cVec3f target = _rContext.playerPosition + Engine::Math::cVec3f(0.0f, 0.2f, 0.0f);
            
            projectile.position = _rEnemy.position + Engine::Math::cVec3f(0.0f, 1.5f * _rEnemy.scale, 0.0f) + _rEnemy.attackDirection * (0.6f * _rEnemy.scale);
            projectile.speed    = 10.5f;
            
            AimMushroomThrow(projectile, target);
            
            projectile.isAreaOfEffect   = true;
            projectile.areaRadius       = _rEnemy.isBoss ? 5.0f : 4.0f;
            projectile.areaDuration     = 4.5f;
            projectile.areaGrowthTime   = 0.9f;
            projectile.damage           = _rDefinition.attackDamage * 2.0f;

            _rProjectileManager.SpawnSpore(projectile);
        }
        else if (_rEnemy.type == World::sEnemyType::ForestCrawler)
        {
            // Three discrete thorns leave gaps to dodge through.
            projectile.damage = _rDefinition.attackDamage * 0.7f;
            projectile.radius = 0.25f;
            for (int thorn = -1; thorn <= 1; ++thorn)
            {
                const float angle = static_cast<float>(thorn) * 0.24f;
                projectile.direction = {
                    _rEnemy.attackDirection.x() * std::cos(angle) + _rEnemy.attackDirection.z() * std::sin(angle),
                    0.0f,
                    _rEnemy.attackDirection.z() * std::cos(angle) - _rEnemy.attackDirection.x() * std::sin(angle)
                };
                _rProjectileManager.SpawnCone(projectile);
            }
        }
        else
        {
            if (_rEnemy.type == World::sEnemyType::ForestThornshooter)
            {
                const Engine::Math::cVec3f target = _rContext.playerPosition + Engine::Math::cVec3f(0.0f, 1.0f, 0.0f);
                projectile.direction = (target - projectile.position).normalized();
            }

            _rProjectileManager.SpawnCone(projectile);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------
