#pragma once 

#include <cstdint>

namespace Engine::GFX
{
    static constexpr uint32_t c_maxNumberOfFrames       = 2; 

    static constexpr uint32_t c_maxNumberOfInstances    = 10000; 
    static constexpr uint32_t c_maxNumberOfLights       = 1000;
    static constexpr uint32_t c_maxNumberOfMaterials    = 1000;

    static constexpr uint32_t c_maxShadowLayers = 16;
}