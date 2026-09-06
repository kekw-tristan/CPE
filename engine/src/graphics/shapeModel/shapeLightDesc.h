#pragma once

#include "graphics/light/light.h"

#include <string>

namespace Engine::GFX
{
    struct sShapeLightDesc
    {
        std::string name;

        sLightType::Enum type = sLightType::Point;

        Math::cVec3f color = { 1.0f, 1.0f, 1.0f };
        float intensity    = 1.0f;

        Math::cVec3f position = { 0.0f, 0.0f, 0.0f };
        float radius          = 1.0f;

        Math::cVec3f direction = { 0.0f, 0.0f, -1.0f };
        float innerConeAngle  = 0.34906585f;
        float outerConeAngle  = 0.52359878f;

        bool castsShadow = false;
    };
}
