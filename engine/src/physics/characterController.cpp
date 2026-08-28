#include "physics/characterController.h"

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

        const float groundHeight = GetGroundHeight();

        m_grounded = m_position.y() <= groundHeight;

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
        m_velocity += Math::cVec3f(0.0f, m_gravity * _deltaTime, 0.0f);

        m_position += m_velocity * _deltaTime;

        const float groundHeight = GetGroundHeight();

        if (m_position.y() <= groundHeight)
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

    float cCharacterController::GetGroundHeight() const
    {
        return 0.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------