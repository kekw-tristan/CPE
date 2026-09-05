#pragma once

#include "physics/collider.h"

#include <random>
#include <vector>

namespace Engine::GFX
{
    class cScene;
}

namespace World
{
    struct sChunk;
    struct sWorldLayout;
    struct sEnemySpawn;

    namespace ForestGenerator
    {
        void GenerateDungeons(
            Engine::GFX::cScene& _rScene,
            const sWorldLayout& _rLayout,
            std::vector<sEnemySpawn>& _rSpawns,
            const sChunk& _rChunk,
            std::vector<Engine::Physics::sAABBCollider>& _rColliders
        );
        void GenerateChunk(
            Engine::GFX::cScene& _rScene,
            const sChunk& _rChunk,
            std::mt19937& _rRandomGenerator,
            sWorldLayout& _rWorldLayout,
            std::vector<sEnemySpawn>& _rEnemySpawns,
            std::vector<Engine::Physics::sAABBCollider>& _rColliders
        );
    }
}
