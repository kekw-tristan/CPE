#include "physics/characterController.h"

#include "physics/collider.h"
#include "physics/collisionWorld.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Physics
{

    // -------------------------------------------------------------------------------------------------------------------------

    cCharacterController::cCharacterController()
        : m_position()
        , m_velocity()
        , m_grounded(true)
        , m_gravity(-9.81f)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCharacterController::SetPosition(const Math::cVec3f& _rPosition)
    {
        m_position = _rPosition;

        float groundHeight = 0.0f;
        m_grounded = FindGroundHeight(m_position.y() + 0.5f, groundHeight) && m_position.y() <= groundHeight;

        if (m_grounded)
        {
            m_position = Math::cVec3f(m_position.x(), groundHeight, m_position.z());
            m_velocity = Math::cVec3f(m_velocity.x(), 0.0f, m_velocity.z());
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCharacterController::Move(const Math::cVec3f& _rDirection, float _speed)
    {
        const Math::cVec3f horizontalVelocity = _rDirection * _speed;

        m_velocity = Math::cVec3f(horizontalVelocity.x(), m_velocity.y(), horizontalVelocity.z());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCharacterController::Jump(float _jumpVelocity)
    {
        if (!m_grounded)
            return;

        m_velocity = Math::cVec3f(m_velocity.x(), _jumpVelocity, m_velocity.z());

        m_grounded = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCharacterController::Update(float _deltaTime)
    {
        constexpr float c_colliderRadius = 0.4f;
        constexpr float c_colliderHalfHeight = 0.8f;

        // Horizontal collision
        const float colliderOffsetY = c_colliderHalfHeight + c_colliderRadius;

        Physics::sCapsuleCollider collider{};

        collider.center = m_position + Math::cVec3f(0.0f, colliderOffsetY, 0.0f);
        collider.radius = c_colliderRadius;
        collider.halfHeight = c_colliderHalfHeight;

        const Math::cVec3f horizontalMovement(
            m_velocity.x() * _deltaTime,
            0.0f,
            m_velocity.z() * _deltaTime
        );

        const Math::cVec3f newColliderCenter = Physics::CollisionWorld::MoveCapsule(collider, horizontalMovement);

        m_position = Math::cVec3f(
            newColliderCenter.x(),
            m_position.y(),
            newColliderCenter.z()
        );

        // Gravity and ground
        const float previousHeight = m_position.y();
        m_velocity += Math::cVec3f(0.0f, m_gravity * _deltaTime, 0.0f);
        m_position += Math::cVec3f(0.0f, m_velocity.y() * _deltaTime, 0.0f);

        constexpr float c_maximumStepHeight = 0.5f;
        float groundHeight = 0.0f;
        const bool groundFound = FindGroundHeight(previousHeight + c_maximumStepHeight, groundHeight);

        if (groundFound && m_position.y() <= groundHeight)
        {
            m_position = Math::cVec3f(m_position.x(), groundHeight, m_position.z());
            m_velocity = Math::cVec3f(m_velocity.x(), 0.0f, m_velocity.z());

            m_grounded = true;
        }
        else
        {
            m_grounded = false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const Math::cVec3f& cCharacterController::GetPosition() const
    {
        return m_position;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const Math::cVec3f& cCharacterController::GetVelocity() const
    {
        return m_velocity;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cCharacterController::IsGrounded() const
    {
        return m_grounded;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCharacterController::SetGravity(float _gravity)
    {
        m_gravity = _gravity;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cCharacterController::GetGravity() const
    {
        return m_gravity;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cCharacterController::FindGroundHeight(float _maximumHeight, float& _rGroundHeight) const
    {
        return Physics::CollisionWorld::FindGroundHeight(m_position, _maximumHeight, _rGroundHeight);
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
