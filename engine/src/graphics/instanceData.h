#pragma once 

#include "math/matrix4x4.h"

#include <array>

namespace Engine::GFX
{
    struct sInstanceFlags
    {
        enum Enum
        {
            InstanceFlagNone,
            InstanceFlagTerrain,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sInstanceData
    {
        Math::cMatrix4x4f worldMatrix{};
        std::array<float, 4>  color;

        int materialIndex;
        int instanceFlags = sInstanceFlags::InstanceFlagNone;
        int padding2;
        int padding3;

    };
}
