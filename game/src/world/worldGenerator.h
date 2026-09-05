#pragma once

#include "math/vector3.h"
#include "graphics/scene/scene.h"
#include "physics/collisionWorld.h"
#include "enemy/enemySpawn.h"

#include <map>
#include <utility>
#include <vector>

namespace World
{
    struct sWorldLayout;

    struct sLoadedChunk
    {
        Engine::GFX::cScene scene;
        std::vector<Engine::Physics::sColliderHandle> colliders;
        std::vector<sEnemySpawn> spawns;
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
