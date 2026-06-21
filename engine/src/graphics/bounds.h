#pragma once 

#include <array> 

namespace Engine::GFX
{
    struct sBounds 
    {
        std::array<float, 3> min;
        std::array<float, 3> max;
        std::array<float, 3> center;
        std::array<float, 3> size;
        float radius;
    };
}