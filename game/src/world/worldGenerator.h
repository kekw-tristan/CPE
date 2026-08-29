#pragma once

#include <vector>

namespace Engine::GFX
{
	class cScene;
}

namespace World 
{

	struct sEnemySpawn;

	namespace WorldGenerator
	{
		void Generate(Engine::GFX::cScene& _rScene, int _seed);
		const std::vector<sEnemySpawn>& GetEnemySpawns();
	}

}