#pragma once 

#include "math/vector3.h"

#include <array>

namespace Engine::GFX
{
    using Engine::Math::cVec3f;

    struct sVertex
    {
        cVec3f position;
        cVec3f normal;
        std::array<float, 2> uv;
    };
}

