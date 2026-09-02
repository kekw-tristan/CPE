#include "collisionDetection.h"

#include <algorithm>
#include <cmath>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Physics
{

    // -------------------------------------------------------------------------------------------------------------------------

    bool IntersectCapsuleAABB(const sCapsuleCollider& _rCapsule, const sAABBCollider& _rBox, sCollisionResult& _rResult)
    {
        const Math::cVec3f boxMin = _rBox.center - _rBox.halfExtents;
        const Math::cVec3f boxMax = _rBox.center + _rBox.halfExtents;

        const float capsuleMinY = _rCapsule.center.y() - _rCapsule.halfHeight;
        const float capsuleMaxY = _rCapsule.center.y() + _rCapsule.halfHeight;

        const float closestBoxX = std::clamp(_rCapsule.center.x(), boxMin.x(), boxMax.x());
        const float closestBoxZ = std::clamp(_rCapsule.center.z(), boxMin.z(), boxMax.z());

        float closestCapsuleY;
        float closestBoxY;

        if (capsuleMaxY < boxMin.y())
        {
            closestCapsuleY = capsuleMaxY;
            closestBoxY     = boxMin.y();
        }
        else if (capsuleMinY > boxMax.y())
        {
            closestCapsuleY = capsuleMinY;
            closestBoxY     = boxMax.y();
        }
        else
        {
            closestCapsuleY = std::clamp(_rCapsule.center.y(), boxMin.y(), boxMax.y());
            closestBoxY     = closestCapsuleY;
        }

        Math::cVec3f difference(
            _rCapsule.center.x() - closestBoxX,
            closestCapsuleY      - closestBoxY,
            _rCapsule.center.z() - closestBoxZ);

        const float distanceSquared =
            difference.x() * difference.x() +
            difference.y() * difference.y() +
            difference.z() * difference.z();

        const float radiusSquared = _rCapsule.radius * _rCapsule.radius;

        if (distanceSquared >= radiusSquared)
        {
            _rResult = {};
            return false;
        }

        _rResult.collided = true;

        const float distance = std::sqrt(distanceSquared);

        if (distance > 0.0001f)
        {
            _rResult.normal           = difference * (1.0f / distance);
            _rResult.penetrationDepth = _rCapsule.radius - distance;

            return true;
        }

        // Capsule axis is already inside the AABB.
        // Choose the nearest horizontal box face.
        const float distanceLeft    = _rCapsule.center.x() - boxMin.x();
        const float distanceRight   = boxMax.x() - _rCapsule.center.x();
        const float distanceBack    = _rCapsule.center.z() - boxMin.z();
        const float distanceFront   = boxMax.z() - _rCapsule.center.z();

        float minDistance = distanceLeft;

        _rResult.normal = Math::cVec3f(-1.0f, 0.0f, 0.0f);

        if (distanceRight < minDistance)
        {
            minDistance = distanceRight;
            _rResult.normal = Math::cVec3f(1.0f, 0.0f, 0.0f);
        }

        if (distanceBack < minDistance)
        {
            minDistance = distanceBack;
            _rResult.normal = Math::cVec3f(0.0f, 0.0f, -1.0f);
        }

        if (distanceFront < minDistance)
        {
            minDistance = distanceFront;
            _rResult.normal = Math::cVec3f(0.0f, 0.0f, 1.0f);
        }

        _rResult.penetrationDepth = _rCapsule.radius + minDistance;

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------