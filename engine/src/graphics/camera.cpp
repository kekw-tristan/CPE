#include "camera.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// -------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr float c_pi = 3.14159265358979323846f;
    constexpr float c_degreesToRadians = c_pi / 180.0f;
    constexpr float c_radiansToDegrees = 180.0f / c_pi;
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    cCamera::cCamera()
        : m_position(0.0f, 0.0f, 0.0f), m_yawDegrees(-90.0f), m_pitchDegrees(0.0f), m_fov(60.0f), m_nearPlane(0.1f), m_farPlane(10000.0f)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::SetPosition(float _x, float _y, float _z) noexcept
    {
        m_position = Engine::Math::cVec3f(_x, _y, _z);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::SetRotation(float _yawDegrees, float _pitchDegrees) noexcept
    {
        m_yawDegrees = _yawDegrees;
        m_pitchDegrees = std::clamp(_pitchDegrees, -89.0f, 89.0f);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::SetPerspective(float _fov, float _nearPlane, float _farPlane) noexcept
    {
        m_fov = _fov;
        m_nearPlane = _nearPlane;
        m_farPlane = _farPlane;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::MoveForward(float _amount) noexcept
    {
        m_position += GetForwardInternal() * _amount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::MoveRight(float _amount) noexcept
    {
        m_position += GetRightInternal() * _amount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::MoveUp(float _amount) noexcept
    {
        m_position += Engine::Math::cVec3f(0.0f, 1.0f, 0.0f) * _amount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::AddYaw(float _amountDegrees) noexcept
    {
        m_yawDegrees += _amountDegrees;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::AddPitch(float _amountDegrees) noexcept
    {
        m_pitchDegrees = std::clamp(m_pitchDegrees + _amountDegrees, -89.0f, 89.0f);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetViewMatrix(float* _pOut16) const noexcept
    {
        if (_pOut16 == nullptr)
        {
            return;
        }

        const Engine::Math::cMatrix4x4f viewMatrix = GetViewMatrixInternal();

        std::memcpy(_pOut16, viewMatrix.data(), sizeof(float) * 16);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetProjectionMatrix(float _aspectRatio, float* _pOut16) const noexcept
    {
        if (_pOut16 == nullptr)
        {
            return;
        }

        const Engine::Math::cMatrix4x4f projectionMatrix = GetProjectionMatrixInternal(_aspectRatio);

        std::memcpy(_pOut16, projectionMatrix.data(), sizeof(float) * 16);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetViewProjectionMatrix(float _aspectRatio, float* _pOut16) const noexcept
    {
        if (_pOut16 == nullptr)
        {
            return;
        }

        const Engine::Math::cMatrix4x4f viewMatrix = GetViewMatrixInternal();
        const Engine::Math::cMatrix4x4f projectionMatrix = GetProjectionMatrixInternal(_aspectRatio);
        const Engine::Math::cMatrix4x4f viewProjectionMatrix = viewMatrix * projectionMatrix;

        std::memcpy(_pOut16, viewProjectionMatrix.data(), sizeof(float) * 16);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetPosition(float* _pOut4) const noexcept
    {
        if (_pOut4 == nullptr)
        {
            return;
        }

        _pOut4[0] = m_position.x();
        _pOut4[1] = m_position.y();
        _pOut4[2] = m_position.z();
        _pOut4[3] = 1.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetDirection(float* _pOut4) const noexcept
    {
        if (_pOut4 == nullptr)
        {
            return;
        }

        const Engine::Math::cVec3f direction = GetForwardInternal();

        _pOut4[0] = direction.x();
        _pOut4[1] = direction.y();
        _pOut4[2] = direction.z();
        _pOut4[3] = 0.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cCamera::GetNearPlane() const noexcept
    {
        return m_nearPlane;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cCamera::GetFarPlane() const noexcept
    {
        return m_farPlane;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::LookAt(float _eyeX, float _eyeY, float _eyeZ, float _targetX, float _targetY, float _targetZ) noexcept
    {
        m_position = Engine::Math::cVec3f(_eyeX, _eyeY, _eyeZ);

        const Engine::Math::cVec3f target(_targetX, _targetY, _targetZ);
        const Engine::Math::cVec3f direction = (target - m_position).normalized();

        if (direction.isZero())
        {
            return;
        }

        const float directionY = std::clamp(direction.y(), -1.0f, 1.0f);

        m_pitchDegrees = std::clamp(std::asin(directionY) * c_radiansToDegrees, -89.0f, 89.0f);
        m_yawDegrees = std::atan2(direction.z(), direction.x()) * c_radiansToDegrees;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    Engine::Math::cMatrix4x4f cCamera::GetViewMatrixInternal() const noexcept
    {
        const Engine::Math::cVec3f forward = GetForwardInternal();
        const Engine::Math::cVec3f worldUp(0.0f, 1.0f, 0.0f);
        const Engine::Math::cVec3f right = forward.cross(worldUp).normalized();
        const Engine::Math::cVec3f up = right.cross(forward);

        return
        {
            right.x(), up.x(), -forward.x(), 0.0f,
            right.y(), up.y(), -forward.y(), 0.0f,
            right.z(), up.z(), -forward.z(), 0.0f,
            -right.dot(m_position), -up.dot(m_position), forward.dot(m_position), 1.0f
        };
    }

    // -------------------------------------------------------------------------------------------------------------------------

    Engine::Math::cMatrix4x4f cCamera::GetProjectionMatrixInternal(float _aspectRatio) const noexcept
    {
        if (_aspectRatio <= 0.0f || m_nearPlane <= 0.0f || m_farPlane <= m_nearPlane)
        {
            return Engine::Math::cMatrix4x4f::identity();
        }

        const float fovRadians = m_fov * c_degreesToRadians;
        const float tangentHalfFov = std::tan(fovRadians * 0.5f);

        if (tangentHalfFov == 0.0f)
        {
            return Engine::Math::cMatrix4x4f::identity();
        }

        const float xScale = 1.0f / (_aspectRatio * tangentHalfFov);
        const float yScale = -1.0f / tangentHalfFov;
        const float depthScale = m_farPlane / (m_nearPlane - m_farPlane);
        const float depthTranslation = -(m_farPlane * m_nearPlane) / (m_farPlane - m_nearPlane);

        return
        {
            xScale, 0.0f, 0.0f, 0.0f,
            0.0f, yScale, 0.0f, 0.0f,
            0.0f, 0.0f, depthScale, -1.0f,
            0.0f, 0.0f, depthTranslation, 0.0f
        };
    }

    // -------------------------------------------------------------------------------------------------------------------------

    Engine::Math::cVec3f cCamera::GetForwardInternal() const noexcept
    {
        const float yawRadians = m_yawDegrees * c_degreesToRadians;
        const float pitchRadians = m_pitchDegrees * c_degreesToRadians;

        const Engine::Math::cVec3f forward
        {
            std::cos(yawRadians) * std::cos(pitchRadians),
            std::sin(pitchRadians),
            std::sin(yawRadians) * std::cos(pitchRadians)
        };

        return forward.normalized();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    Engine::Math::cVec3f cCamera::GetRightInternal() const noexcept
    {
        const Engine::Math::cVec3f worldUp(0.0f, 1.0f, 0.0f);

        return GetForwardInternal().cross(worldUp).normalized();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}