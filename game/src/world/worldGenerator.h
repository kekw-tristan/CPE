#pragma once

#include "math/vector3.h"
#include "graphics/scene/scene.h"
#include "physics/collisionWorld.h"
#include "enemy/enemySpawn.h"

#include <map>
#include <cstdint>
#include <utility>
#include <vector>

namespace World
{
    struct sWorldLayout;

    struct sReflectionProbeDesc
    {
        Engine::Math::cVec3f position;
        Engine::Math::cVec3f boxMin;
        Engine::Math::cVec3f boxMax;

        float blendDistance = 8.0f;
        uint32_t resolution = 64;
    };

    struct sLoadedChunk
    {
        Engine::GFX::cScene scene;
        std::vector<Engine::Physics::sColliderHandle> colliders;
        std::vector<sEnemySpawn> spawns;
        std::vector<sReflectionProbeDesc> reflectionProbes;
    };

    namespace WorldGenerator
    {
        void Generate(int _seed);

        // The startup budget fills the whole window; normal updates load one chunk.
        bool Update(const Engine::Math::cVec3f& _rPosition, int _chunkBudget = 1);

        const std::map<std::pair<int, int>, sLoadedChunk>& GetLoadedChunks();
        const sWorldLayout& GetLayout();
        void Clear();
    }
}
