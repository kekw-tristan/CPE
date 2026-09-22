#pragma once

#include "collider.h"

namespace Engine::Physics
{
    struct sCollisionResult
    {
        bool collided = false;

        Math::cVec3f normal;
        float penetrationDepth = 0;
    };

    bool IntersectCapsuleAABB(const sCapsuleCollider& _rCapsule, const sAABBCollider& _rBox, sCollisionResult& _rResult);
    bool IntersectCapsuleTriangle(const sCapsuleCollider& _rCapsule, const sTriangleCollider& _rTriangle, sCollisionResult& _rResult);
    bool FindTriangleGroundHeight(const Math::cVec3f& _rPosition, const sTriangleCollider& _rTriangle, float& _rHeight);

    // Earliest contact along a segment: 0 for initial overlap, 1 for an unobstructed segment.
    // AABB corners are conservatively expanded by the radius.
    float SweepSphereAABB(const Math::cVec3f& _rStart, const Math::cVec3f& _rMovement,
        float _radius, const sAABBCollider& _rBox);
    float SweepSphereTriangle(const Math::cVec3f& _rStart, const Math::cVec3f& _rMovement,
        float _radius, const sTriangleCollider& _rTriangle);
}
