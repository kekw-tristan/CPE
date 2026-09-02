#pragma once

#include "math/vector3.h"

namespace Engine::Physics
{
	struct sAABBCollider
	{
		Math::cVec3f center; 
		Math::cVec3f halfExtents; 
	};

	struct sCapsuleCollider
	{
		Math::cVec3f center; 

		float radius;
		float halfHeight;
	};
}