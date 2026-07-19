#pragma once

#include "math/vector3.h"

namespace Engine::GFX
{
    using Engine::Math::cVec3f;

    struct sBounds
    {
        cVec3f min{};
        cVec3f max{};
        cVec3f center{};
        cVec3f size{};

        float radius = 0.0f;
    };
}