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
}