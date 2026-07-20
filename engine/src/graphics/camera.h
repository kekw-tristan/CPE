#pragma once

#include "Math/matrix4x4.h"
#include "Math/vector3.h"

namespace Engine::GFX
{
    class cCamera
    {

        public:

            cCamera();
            ~cCamera() = default;

        public:

            void SetPosition(float _x, float _y, float _z) noexcept;
            void SetRotation(float _yawDegrees, float _pitchDegrees) noexcept;
            void SetPerspective(float _fov, float _nearPlane, float _farPlane) noexcept;

            void MoveForward(float _amount) noexcept;
            void MoveRight(float _amount) noexcept;
            void MoveUp(float _amount) noexcept;

            void AddYaw(float _amountDegrees) noexcept;
            void AddPitch(float _amountDegrees) noexcept;

            void GetViewMatrix(float* _pOut16) const noexcept;
            void GetProjectionMatrix(float _aspectRatio, float* _pOut16) const noexcept;
            void GetViewProjectionMatrix(float _aspectRatio, float* _pOut16) const noexcept;

            void GetPosition(float* _pOut4) const noexcept;
            void GetDirection(float* _pOut4) const noexcept;

            float GetNearPlane() const noexcept;
            float GetFarPlane() const noexcept;

            void LookAt(float _eyeX, float _eyeY, float _eyeZ, float _targetX, float _targetY, float _targetZ) noexcept;

        private:

            Engine::Math::cMatrix4x4f GetViewMatrixInternal() const noexcept;
            Engine::Math::cMatrix4x4f GetProjectionMatrixInternal(float _aspectRatio) const noexcept;

            Engine::Math::cVec3f GetForwardInternal() const noexcept;
            Engine::Math::cVec3f GetRightInternal() const noexcept;

        private:

            Engine::Math::cVec3f m_position{};

            float m_yawDegrees{};
            float m_pitchDegrees{};
            float m_fov{};
            float m_nearPlane{};
            float m_farPlane{};
    };
}