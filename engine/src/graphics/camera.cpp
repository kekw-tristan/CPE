#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstring>
#include <cmath>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    cCamera::cCamera()
        : m_position(glm::vec3(0.f, 0.f, 0.f))
        , m_yawDegrees(-90.f)
        , m_pitchDegrees(0.f)
        , m_fov(60.f)
        , m_nearPlane(0.1f) 
        , m_farPlane(10000.f)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::SetPosition(float _x, float _y, float _z)
    {
        m_position = glm::vec3(_x, _y, _z);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::SetRotation(float _yawDegrees, float _pitchDegrees)
    {
        m_yawDegrees = _yawDegrees;
        m_pitchDegrees = std::clamp(_pitchDegrees, -89.f, 89.f);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::SetPerspective(float _fov, float _nearPlane, float _farPlane)
    {
        m_fov = _fov;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::MoveForward(float _amount)
    {
        m_position += GetForwardInternal() * _amount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::MoveRight(float _amount)
    {
        m_position += GetRightInternal() * _amount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::MoveUp(float _amount)
    {
        m_position += glm::vec3(0.0f, 1.0f, 0.0f) * _amount;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::AddYaw(float _amountDegrees)
    {
        m_yawDegrees += _amountDegrees;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::AddPitch(float _amountDegrees)
    {
        m_pitchDegrees += _amountDegrees;
        m_pitchDegrees = std::clamp(m_pitchDegrees, -89.0f, 89.0f);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetViewMatrix(float* _pOut16) const
    {
        glm::mat4 viewMatrix = glm::lookAt(m_position, m_position + GetForwardInternal(), glm::vec3(0.0f, 1.0f, 0.0f));

        std::memcpy(_pOut16, glm::value_ptr(viewMatrix), sizeof(float) * 16);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetProjectionMatrix(float _aspectRatio, float *_pOut16) const
    {
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(m_fov), _aspectRatio, m_nearPlane, m_farPlane);

        projectionMatrix[1][1] *= -1.0f;

        std::memcpy(_pOut16, glm::value_ptr(projectionMatrix), sizeof(float) * 16);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetViewProjectionMatrix(float _aspectRatio, float *_pOut16) const
    {
         glm::mat4 viewMatrix = glm::lookAt(m_position, m_position + GetForwardInternal(), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projectionMatrix = glm::perspective(glm::radians(m_fov), _aspectRatio, m_nearPlane, m_farPlane);

        projectionMatrix[1][1] *= -1.0f;

        glm::mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

        std::memcpy(_pOut16, glm::value_ptr(viewProjectionMatrix), sizeof(float) * 16);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetPosition(float *_pOut4) const
    {
        _pOut4[0] = m_position.x;
        _pOut4[1] = m_position.y;
        _pOut4[2] = m_position.z;
        _pOut4[3] = 1.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::GetDirection(float *_pOut4) const
    {
        const glm::vec3 direction = GetForwardInternal();

        _pOut4[0] = direction.x;
        _pOut4[1] = direction.y;
        _pOut4[2] = direction.z;
        _pOut4[3] = 0.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cCamera::GetNearPlane() const
    {
        return m_nearPlane;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cCamera::GetFarPlane() const
    {
        return m_farPlane;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cCamera::LookAt(float _eyeX, float _eyeY, float _eyeZ, float _targetX, float _targetY, float _targetZ)
    {
        m_position = glm::vec3(_eyeX, _eyeY, _eyeZ);

        const glm::vec3 target      = glm::vec3(_targetX, _targetY, _targetZ);
        const glm::vec3 direction   = glm::normalize(target - m_position);
        
        m_pitchDegrees  = glm::degrees(std::asin(direction.y));
        m_yawDegrees    = glm::degrees(std::atan2(direction.z, direction.x));

    }

    // -------------------------------------------------------------------------------------------------------------------------

    glm::vec3 cCamera::GetForwardInternal() const
    {
        const float yawRadians      = glm::radians(m_yawDegrees);
        const float pitchRadians    = glm::radians(m_pitchDegrees);

        glm::vec3 forward{};

        forward.x = std::cos(yawRadians) * std::cos(pitchRadians);
        forward.y = std::sin(pitchRadians);
        forward.z = std::sin(yawRadians) * std::cos(pitchRadians);

        return glm::normalize(forward);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    glm::vec3 cCamera::GetRightInternal() const
    {
        return glm::normalize(glm::cross(GetForwardInternal(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------