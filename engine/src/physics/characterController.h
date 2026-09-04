#pragma once

#include "math/vector3.h"

namespace Engine::Physics
{

    class cCharacterController
    {

    public:

        cCharacterController();

        void SetPosition(const Math::cVec3f& _rPosition);

        void Move(const Math::cVec3f& _rDirection, float _speed);
        void Jump(float _jumpVelocity);

        void Update(float _deltaTime);

        const Math::cVec3f& GetPosition() const;
        const Math::cVec3f& GetVelocity() const;

        bool IsGrounded() const;

        void SetGravity(float _gravity);
        float GetGravity() const;


    private:

        bool FindGroundHeight(float _maximumHeight, float& _rGroundHeight) const;


    private:

        Math::cVec3f m_position;
        Math::cVec3f m_velocity;

        bool m_grounded;

        float m_gravity;

    };

}
