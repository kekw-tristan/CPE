#pragma once

#include "math/vector3.h"

#include "biome.h"

#include <vector>

namespace World
{
	struct sChunkCoordinate
	{
		int x = 0; 
		int y = 0; 
		int z = 0;
	};

	struct sChunk
	{
		sChunkCoordinate coordinate{};

		sBiomeType::Enum biome = sBiomeType::Undefined;

		float height	 = 0.f; 
		float biomeBlend = 0.f;
	};

	struct sPathPoint
	{
		Engine::Math::cVec3f position;
	};

	struct sWorldLayout
	{
		std::vector<sPathPoint> mainPath;
	};
}