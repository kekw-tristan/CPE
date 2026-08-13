#pragma once

#include "math/vector3.h"

namespace Engine::GFX
{
    struct sLightType
    {
        enum Enum
        {
            Directional,
            Point,
            Spot,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sLight
    {
        sLightType::Enum type = sLightType::Point;
        
        Math::cVec3f color      = { 1.f, 1.f, 1.f };
        float intensity         = 1.f; 

        Math::cVec3f position   = { 0.f, 0.f, -1.f }; 
        float radius            = 1.f; 

        Math::cVec3f direction  = { 0.f, 0.f, -1.f };
        float innerCone         = 0.f;

        float outerCone         = 0.f;

    };

    struct sLightGPU
    {
        float positionRadius[4];
        float directionType[4];
        float colorIntensity[4];
        float spotData[4];
    };
}