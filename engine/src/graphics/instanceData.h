#pragma once 

#include "math/matrix4x4.h"

#include <array>

namespace Engine::GFX
{
    struct sInstanceData
    {
        Math::cMatrix4x4f worldMatrix{};
        std::array<float, 4>  color;
    };
}