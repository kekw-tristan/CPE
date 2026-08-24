#pragma once

#include "biome.h"

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
}