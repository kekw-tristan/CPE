#pragma once

#include <random>

namespace Engine::GFX
{
    class cScene;
}

namespace World
{
    struct sChunk;

    namespace ForestGenerator
    {
        void GenerateChunk(Engine::GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator);
    }
}