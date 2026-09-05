#pragma once

#include "math/vector3.h"

namespace Engine::Physics
{
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
