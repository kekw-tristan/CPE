#include "worldGenerator.h"

#include "chunk.h"

#include "graphics/scene/scene.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "physics/collisionWorld.h"

#include "worldModels.h"

#include "biome/forestGenerator.h"

#include "enemy/enemySpawn.h"

#include <random>
#include <iostream>
#include <vector>

using namespace Engine;

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{

	// -------------------------------------------------------------------------------------------------------------------------

	namespace
	{
		constexpr int c_chunkSize = 32;

		constexpr int c_worldChunkCountX = 8;
		constexpr int c_worldChunkCountZ = 8;
	}

	// -------------------------------------------------------------------------------------------------------------------------

	namespace
	{
		class cWorldGenerator
		{

		public:

			static cWorldGenerator& GetInstance();

		public:

			void Generate(GFX::cScene& _rScene, int _seed);
			const std::vector<sEnemySpawn>& GetEnemySpawns();

		private:

			cWorldGenerator();
			~cWorldGenerator();

			cWorldGenerator(const cWorldGenerator&)				= delete;
			cWorldGenerator& operator=(const cWorldGenerator&)	= delete;

			cWorldGenerator(const cWorldGenerator&&)			= delete;
			cWorldGenerator& operator=(const cWorldGenerator&&) = delete;

		private:

			void GenerateLayout();

			void GenerateChunk(GFX::cScene& _rScene, const sChunk& _rChunk);

		private:

			std::mt19937 m_randomGenerator;

			std::vector<sChunk> m_chunks;

			sWorldLayout m_layout;

			std::vector<sEnemySpawn> m_enemySpawns;

		};

	}

	// -------------------------------------------------------------------------------------------------------------------------

	namespace
	{

		// -------------------------------------------------------------------------------------------------------------------------

		cWorldGenerator& cWorldGenerator::GetInstance()
		{
			static cWorldGenerator s_instance;
			return s_instance;
		}

		// -------------------------------------------------------------------------------------------------------------------------

		void cWorldGenerator::Generate(GFX::cScene& _rScene, int _seed)
		{
			_rScene.Clear();
			m_enemySpawns.clear();
			Physics::CollisionWorld::Clear();

			m_randomGenerator.seed(_seed);

			GenerateLayout();

			for (const sChunk& chunk : m_chunks)
				GenerateChunk(_rScene, chunk);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		const std::vector<sEnemySpawn>& cWorldGenerator::GetEnemySpawns()
		{
			return m_enemySpawns;
		}

		// -------------------------------------------------------------------------------------------------------------------------

		cWorldGenerator::cWorldGenerator()
			: m_randomGenerator()
		{
			if (!WorldModels::Load("./assets/models"))
				std::cerr << "One or more world models could not be loaded.\n";
		}

		// -------------------------------------------------------------------------------------------------------------------------

		cWorldGenerator::~cWorldGenerator()
		{
		}

		// -------------------------------------------------------------------------------------------------------------------------

		void cWorldGenerator::GenerateLayout()
		{
			m_chunks.clear();

			const int startChunkX = -(c_worldChunkCountX / 2);
			const int startChunkZ = -(c_worldChunkCountZ / 2);

			m_chunks.reserve(c_worldChunkCountX * c_worldChunkCountZ);

			m_layout.mainPath =
			{
				{ Math::cVec3f(-100.0f, 0.0f, -80.0f) },
				{ Math::cVec3f(-60.0f, 0.0f, -40.0f) },
				{ Math::cVec3f(-20.0f, 0.0f, -10.0f) },
				{ Math::cVec3f(20.0f, 0.0f,  30.0f) },
				{ Math::cVec3f(70.0f, 0.0f,  50.0f) }
			};

			for (int z = 0; z < c_worldChunkCountZ; ++z)
			{
				for (int x = 0; x < c_worldChunkCountX; ++x)
				{
					sChunk chunk{};

					chunk.coordinate = { startChunkX + x, 0, startChunkZ + z };
					chunk.biome = sBiomeType::Forest;
					chunk.height = 0.0f;
					chunk.biomeBlend = 0.0f;

					m_chunks.push_back(chunk);
				}
			}
		}

		// -------------------------------------------------------------------------------------------------------------------------

		void cWorldGenerator::GenerateChunk(GFX::cScene& _rScene, const sChunk& _rChunk)
		{

			switch (_rChunk.biome)
			{
				case sBiomeType::Forest:
					ForestGenerator::GenerateChunk(_rScene, _rChunk, m_randomGenerator, m_layout, m_enemySpawns);
					break;

				case sBiomeType::Swamp:
					break;

				case sBiomeType::Ice:
					break;

				case sBiomeType::Lava:
					break;
			}
		}

		// -------------------------------------------------------------------------------------------------------------------------

	}

	// -------------------------------------------------------------------------------------------------------------------------

	namespace WorldGenerator
	{

		// -------------------------------------------------------------------------------------------------------------------------

		void Generate(Engine::GFX::cScene& _rScene, int _seed)
		{
			cWorldGenerator::GetInstance().Generate(_rScene, _seed);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		const std::vector<sEnemySpawn>& GetEnemySpawns()
		{
			return cWorldGenerator::GetInstance().GetEnemySpawns();
		}

		// -------------------------------------------------------------------------------------------------------------------------

	}

	// -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------

