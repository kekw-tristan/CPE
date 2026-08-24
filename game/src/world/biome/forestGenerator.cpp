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

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ForestGenerator
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateChunk(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator)
        {
            GenerateGround(_rScene, _rChunk);

            (void)_rRandomGenerator;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}