#pragma once 

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>

namespace Engine::GFX
{
    class cCamera
    {
        public:

            cCamera();
           ~cCamera() = default;

        public:

            void SetPosition(float _x, float _y, float _z);
            void SetRotation(float _yawDegrees, float _pitchDegrees);
            void SetPerspective(float _fov, float _nearPlane, float _farPlane);

            void MoveForward(float _amount);
            void MoveRight(float _amount); 
            void MoveUp(float _amount); 

            void AddYaw(float _amountDegrees);
            void AddPitch(float _amountDegrees);

            void GetViewMatrix(float* _pOut16) const;
            void GetProjectionMatrix(float _aspectRatio, float* _pOut16) const;
            void GetViewProjectionMatrix(float _aspectRatio, float* _pOut16) const;

            void GetPosition(float* _pOut4) const;
            void GetDirection(float* _pOut4) const;

            float GetNearPlane() const;
            float GetFarPlane() const;

            void LookAt(float _eyeX, float _eyeY, float _eyeZ, float _targetX, float _targetY, float _targetZ);

        private:

            glm::vec3 GetForwardInternal() const;
            glm::vec3 GetRightInternal() const;

        private:

            glm::vec3 m_position;

            float m_yawDegrees;
            float m_pitchDegrees;
            float m_fov; 
            float m_nearPlane; 
            float m_farPlane;
    };
}