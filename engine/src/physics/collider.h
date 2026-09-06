#pragma once

#include "math/vector3.h"

namespace Engine::Physics
{
    // Static world-space surface. Contact is two-sided; winding defines the walkable side.
    struct sTriangleCollider
    {
        Math::cVec3f a;
        Math::cVec3f b;
        Math::cVec3f c;
    };

    struct sAABBCollider
    {
        Math::cVec3f center;
        Math::cVec3f halfExtents;
        bool isGround = false;

        // Optional height offset from the box top, sampled at world X/Z coordinates.
        float (*groundHeightSampler)(float, float) = nullptr;
    };

	struct sCapsuleCollider
	{
		Math::cVec3f center; 

		float radius;
		float halfHeight;
	};
}
