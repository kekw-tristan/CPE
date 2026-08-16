#pragma once

#include "graphics/camera.h"

#include <limits>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Math
{

    // -------------------------------------------------------------------------------------------------------------------------

    std::array<Math::cVec3f, 8> CalculateFrustumCorners(const GFX::cCamera& _rCamera, const float _aspectRatio, const float _nearDistance, const float _farDistance)
    {
        float cameraPositionData[4];
        float cameraDirectionData[4];
        float projectionData[16];

        _rCamera.GetPosition(cameraPositionData);
        _rCamera.GetDirection(cameraDirectionData);
        _rCamera.GetProjectionMatrix(_aspectRatio, projectionData);

        const Math::cVec3f position =
        {
            cameraPositionData[0],
            cameraPositionData[1],
            cameraPositionData[2]
        };

        const Math::cVec3f forward = Math::cVec3f(
            cameraDirectionData[0],
            cameraDirectionData[1],
            cameraDirectionData[2]
        ).normalized();

        const Math::cVec3f worldUp  = { 0.0f, 1.0f, 0.0f };
        const Math::cVec3f right    = forward.cross(worldUp).normalized();
        const Math::cVec3f up       = right.cross(forward).normalized();

        const float xScale = std::abs(projectionData[0]);
        const float yScale = std::abs(projectionData[5]);

        if (xScale <= 0.000001f || yScale <= 0.000001f)
        {
            return {};
        }

        const float nearHalfWidth   = _nearDistance / xScale;
        const float nearHalfHeight  = _nearDistance / yScale;

        const float farHalfWidth    = _farDistance / xScale;
        const float farHalfHeight   = _farDistance / yScale;

        const Math::cVec3f nearCenter = position + forward * _nearDistance;
        const Math::cVec3f farCenter  = position + forward * _farDistance;

        std::array<Math::cVec3f, 8> corners{};

        // Near plane
        corners[0] = nearCenter - right * nearHalfWidth + up * nearHalfHeight; // top left
        corners[1] = nearCenter + right * nearHalfWidth + up * nearHalfHeight; // top right
        corners[2] = nearCenter - right * nearHalfWidth - up * nearHalfHeight; // bottom left
        corners[3] = nearCenter + right * nearHalfWidth - up * nearHalfHeight; // bottom right

        // Far plane
        corners[4] = farCenter - right * farHalfWidth + up * farHalfHeight; // top left
        corners[5] = farCenter + right * farHalfWidth + up * farHalfHeight; // top right
        corners[6] = farCenter - right * farHalfWidth - up * farHalfHeight; // bottom left
        corners[7] = farCenter + right * farHalfWidth - up * farHalfHeight; // bottom right

        return corners;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    Math::cMatrix4x4f CalculateDirectionalShadowMatrix(const std::array<Math::cVec3f, 8>& _rCorners, Math::cVec3f _lightDirection)
    {
        _lightDirection = _lightDirection.normalized();

        Math::cVec3f center{ 0.f, 0.f, 0.f };

        for (const Math::cVec3f& corner : _rCorners)
        {
            center += corner;
        }

        center /= static_cast<float>(_rCorners.size());

        float radius = 0.f;

        for (const Math::cVec3f& corner : _rCorners)
        {
            const Math::cVec3f offset = corner - center;
            radius = std::max(radius, offset.length());
        }

        Math::cVec3f up = { 0.f, 1.f, 0.f };

        if (std::abs(_lightDirection.y()) > 0.99f)
        {
            up = { 0.f, 0.f, 1.f };
        }

        const float depthPadding  = 10.f;
        const float lightDistance = radius + depthPadding;

        const Math::cVec3f      lightPosition   = center - _lightDirection * lightDistance;
        const Math::cMatrix4x4f lightView       = Math::cMatrix4x4f::lookAtRH(lightPosition, center, up);

        float minX =  std::numeric_limits<float>::max();
        float maxX = -std::numeric_limits<float>::max();
        float minY =  std::numeric_limits<float>::max();
        float maxY = -std::numeric_limits<float>::max();
        float minZ =  std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();

        for (const Math::cVec3f& corner : _rCorners)
        {
            const Math::cVec3f lightSpaceCorner = lightView.transformPoint(corner);

            minX = std::min(minX, lightSpaceCorner.x());
            maxX = std::max(maxX, lightSpaceCorner.x());

            minY = std::min(minY, lightSpaceCorner.y());
            maxY = std::max(maxY, lightSpaceCorner.y());

            minZ = std::min(minZ, lightSpaceCorner.z());
            maxZ = std::max(maxZ, lightSpaceCorner.z());
        }

        const float xyPadding = 1.f;

        minX -= xyPadding;
        maxX += xyPadding;
        minY -= xyPadding;
        maxY += xyPadding;

        const float nearPlane = std::max(0.1f, -maxZ - depthPadding);
        const float farPlane  = std::max(nearPlane + 0.1f, -minZ + depthPadding);

        const Math::cMatrix4x4f lightProjection = Math::cMatrix4x4f::orthographicRH(minX, maxX, minY, maxY, nearPlane, farPlane);

        return lightView * lightProjection;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------