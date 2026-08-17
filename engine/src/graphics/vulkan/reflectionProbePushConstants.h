#pragma once

#include "math/matrix4x4.h"

namespace Engine::GFX
{
    struct sReflectionProbePushConstants
    {
        Math::cMatrix4x4f viewProjection;

        float cameraPosition[4];
    };

    static_assert(sizeof(sReflectionProbePushConstants) == 80);
}