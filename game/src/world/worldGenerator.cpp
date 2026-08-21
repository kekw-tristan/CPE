#include "worldGenerator.h"

#include "chunk.h"

#include "graphics/scene/scene.h"

#include "graphics/shapeModel/shapeMeshLibrary.h"
#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapePartDesc.h"

#include <random>
#include <iostream>

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

			private:

				cWorldGenerator(); 
			   ~cWorldGenerator();

			   cWorldGenerator(const cWorldGenerator&)				= delete; 
			   cWorldGenerator& operator=(const cWorldGenerator&)	= delete;

			   cWorldGenerator(const cWorldGenerator&&)				= delete;
			   cWorldGenerator& operator=(const cWorldGenerator&&)	= delete;

			private:

				void GenerateChunk(GFX::cScene& _rScene, const sChunkCoordinate& _rCoordinate);

				void GenerateChunkGround(GFX::cScene& _rScene, const sChunkCoordinate& _rCoordinate);
				
				void InitModels(); 

			private:

				std::mt19937 m_randomGenerator;

			private:

				GFX::ShapeModelHandle m_groundModelHandle;

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

			m_randomGenerator.seed(_seed);

			const int startChunkX = -(c_worldChunkCountX / 2);
			const int startChunkZ = -(c_worldChunkCountZ / 2);
			
			for (int z = 0; z < c_worldChunkCountZ; ++z)
			{
				for (int x = 0; x < c_worldChunkCountX; ++x)
				{
					GenerateChunk(_rScene, { startChunkX + x, 0, startChunkZ + z});
				}
			}
		}

		// -------------------------------------------------------------------------------------------------------------------------

		cWorldGenerator::cWorldGenerator()
			: m_randomGenerator()
			, m_groundModelHandle(-1)
		{
			InitModels();
		}

		// -------------------------------------------------------------------------------------------------------------------------

		cWorldGenerator::~cWorldGenerator()
		{
		}

		// -------------------------------------------------------------------------------------------------------------------------

		void cWorldGenerator::GenerateChunk(GFX::cScene& _rScene, const sChunkCoordinate& _rCoordinate)
		{
			GenerateChunkGround(_rScene, _rCoordinate);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		void cWorldGenerator::GenerateChunkGround(GFX::cScene& _rScene, const sChunkCoordinate& _rCoordinate)
		{
			const float worldX = static_cast<float>(_rCoordinate.x * c_chunkSize);
			const float worldZ = static_cast<float>(_rCoordinate.z * c_chunkSize);

			GFX::sShapeInstance groundInstance{};

			groundInstance.modelHandle = m_groundModelHandle;

			groundInstance.transform.position	= Math::cVec3f(worldX, 0.0f, worldZ);
			groundInstance.transform.rotation	= Math::cVec3f(0.0f, 0.0f, 0.0f);
			groundInstance.transform.scale		= Math::cVec3f(1.0f, 1.0f, 1.0f);

			_rScene.AddShapeInstance(groundInstance);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		void cWorldGenerator::InitModels()
		{
			GFX::sShapeModelDesc groundModel{};

			groundModel.pDebugName = "Generated Ground";

			GFX::sShapePartDesc groundPart{};

			groundPart.name = "Ground";
			groundPart.meshType = GFX::sMeshTypes::ChunkPlane;

			groundPart.transform.position	= Math::cVec3f(0.0f, 0.0f, 0.0f);
			groundPart.transform.rotation	= Math::cVec3f(0.0f, 0.0f, 0.0f);
			groundPart.transform.scale		= Math::cVec3f(1.0f, 1.0f, 1.0f);

			groundPart.color[0] = 0.0f;
			groundPart.color[1] = 1.0f;
			groundPart.color[2] = 0.0f;
			groundPart.color[3] = 1.0f;

			groundPart.materialIndex = 0;

			groundModel.shapes.push_back(groundPart);
			groundModel.materialIndices.push_back(0);

			groundModel.bounds = GFX::ShapeMeshLibrary::GetBounds(GFX::sMeshTypes::ChunkPlane);

			m_groundModelHandle = GFX::ShapeModelManager::CreateShapeModel(groundModel);
		}
	}

	// -------------------------------------------------------------------------------------------------------------------------

	namespace WorldGenerator
	{
		void Generate(Engine::GFX::cScene& _rScene, int _seed)
		{
			cWorldGenerator::GetInstance().Generate(_rScene, _seed);
		}
	}

	// -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------

