#pragma once

#include "math/vector3.h"

#include "graphics/gfxConfig.h"

#include <cstdint>

namespace Engine::GFX
{

    struct sReflectionProbeProjectionType
    {
        enum Enum
        {
            Infinite,
            Box,

            NumberOfElements,
            Undefined = -1
        };
    };


    struct sReflectionProbe
    {
        Math::cVec3f position;

        Math::cVec3f boxMin;
        Math::cVec3f boxMax;

        float radius = 30.0f;
        float blendDistance = 2.0f;

        uint32_t resolution = 256;

        sReflectionProbeProjectionType::Enum projectionType = sReflectionProbeProjectionType::Infinite;

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