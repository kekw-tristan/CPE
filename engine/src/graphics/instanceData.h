#pragma once 

#include <array>
namespace Engine::GFX
{
    struct sInstanceData
    {
        std::array<float, 16> worldMatrix;
        std::array<float, 4>  color;
    };
}