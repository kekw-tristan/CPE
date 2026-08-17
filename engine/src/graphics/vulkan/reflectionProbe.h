#pragma once

#include "math/vector3.h"

#include "graphics/gfxConfig.h"

#include <cstdint>

namespace Engine::GFX
{

    struct sReflectionProbe
    {
        Math::cVec3f position = { 0.f, 3.f, 0.f };

        Math::cVec3f boxMin = { -20.f, 0.f, -20.f };
        Math::cVec3f boxMax = { 20.f, 10.f, 20.f };

        float radius = 30.f;
        float blendDistance = 3.f;

        uint32_t resolution = 256;

        bool dirty = true;
    };

    struct sReflectionProbeGPU
    {
        // xyz = position
        // w   = maximum prefiltered mip
        float positionMaxMip[4];

        // xyz = box min
        // w   = blend distance
        float boxMinBlendDistance[4];

        // xyz = box max
        // w   = unused
        float boxMax[4];
    };

    static_assert(sizeof(sReflectionProbeGPU) == 48);
}