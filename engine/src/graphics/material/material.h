#pragma once

#include "math/vector3.h"

namespace Engine::GFX
{
    struct sMaterial
    {
        Math::cVec3f albedo = { 1.f, 1.f, 1.f };
        float ambientStrength = 1.f;

        float roughness = 0.5f;
        float metallic = 0.f;
        float lightWrap = 0.f;
        float shapeContrast = 1.f;

        Math::cVec3f emissiveColor = { 0.f, 0.f, 0.f };
        float emissiveStrength = 0.f;
    };

    static_assert(sizeof(sMaterial) == 48);
}