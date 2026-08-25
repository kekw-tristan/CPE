#include "forestGenerator.h"

#include "../chunk.h"
#include "../worldConfig.h"
#include "../worldModels.h"

#include "graphics/scene/scene.h"

#include <random>

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{

    using namespace Engine;

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        
        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateGround(GFX::cScene& _rScene, const sChunk& _rChunk)
        {
            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            GFX::sShapeInstance groundInstance{};

            groundInstance.modelHandle = WorldModels::Get("ground");

            groundInstance.transform.position = Math::cVec3f(worldX, worldY, worldZ);
            groundInstance.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
            groundInstance.transform.scale    = Math::cVec3f(1.0f, 1.0f, 1.0f);

            _rScene.AddShapeInstance(groundInstance);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateTrees(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator)
        {
            constexpr uint32_t c_minTreeCount = 20;
            constexpr uint32_t c_maxTreeCount = 40;

            constexpr float c_treeBorder    = 0.5f;
            constexpr float c_minScale      = 0.85f;
            constexpr float c_maxScale      = 1.15f;
            constexpr float c_twoPi         = 6.28318530718f;

            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height + 1;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            const float halfChunkSize   = static_cast<float>(c_chunkSize) * 0.5f;
            const float minOffset       = -halfChunkSize + c_treeBorder;
            const float maxOffset       = halfChunkSize - c_treeBorder;

            std::uniform_int_distribution<uint32_t> treeCountDistribution(c_minTreeCount, c_maxTreeCount);
            std::uniform_real_distribution<float>   positionDistribution(minOffset, maxOffset);
            std::uniform_real_distribution<float>   rotationDistribution(0.0f, c_twoPi);
            std::uniform_real_distribution<float>   scaleDistribution(c_minScale, c_maxScale);

            const uint32_t treeCount = treeCountDistribution(_rRandomGenerator);

            for (uint32_t i = 0; i < treeCount; ++i)
            {
                const float treeX        = worldX + positionDistribution(_rRandomGenerator);
                const float treeZ        = worldZ + positionDistribution(_rRandomGenerator);
                const float treeRotation = rotationDistribution(_rRandomGenerator);
                const float treeScale    = scaleDistribution(_rRandomGenerator);

                GFX::sShapeInstance treeInstance{};

                treeInstance.modelHandle = WorldModels::Get("tree");

                treeInstance.transform.position = Math::cVec3f(treeX, worldY, treeZ);
                treeInstance.transform.rotation = Math::cVec3f(0.0f, treeRotation, 0.0f);
                treeInstance.transform.scale = Math::cVec3f(treeScale, treeScale, treeScale);

                _rScene.AddShapeInstance(treeInstance);
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ForestGenerator
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateChunk(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator)
        {
            GenerateGround(_rScene, _rChunk);
            GenerateTrees(_rScene, _rChunk, _rRandomGenerator);

            (void)_rRandomGenerator;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------