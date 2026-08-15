#pragma once

#include "math/matrix4x4.h"

namespace Engine::GFX
{
	struct sShadowDataGPU
	{
		Math::cMatrix4x4f viewProjection[6];

		uint32_t lightIndex;
		uint32_t firstLayer; 
		uint32_t matrixCount; 
		uint32_t padding;
	};

	struct sShadowPushConstants
	{
		uint32_t shadowIndex;
		uint32_t matrixIndex;
	};
}