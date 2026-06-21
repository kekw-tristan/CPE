#pragma once 

#include <array>

namespace Engine::GFX
{
    struct sVertex
    {
        std::array<float, 3> position;
        std::array<float, 3> normal;
        std::array<float, 2> uv;
        std::array<float, 4> color;
    };
}

