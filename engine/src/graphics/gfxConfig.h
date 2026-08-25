#pragma once 

#include <cstdint>

namespace Engine::GFX
{
    static constexpr uint32_t c_maxNumberOfFrames       = 2; 

    static constexpr uint32_t c_maxNumberOfInstances    = 10000; 
    static constexpr uint32_t c_maxNumberOfLights       = 1000;
    static constexpr uint32_t c_maxNumberOfMaterials    = 1000;

    static constexpr uint32_t c_maxShadowLayers = 16;

    static constexpr uint32_t c_directionalCascadeCount = 3;

    static constexpr float c_directionalCascadeSplits[c_directionalCascadeCount] =
    {
        25.f,
        70.f,
        150.f,
    };

    static constexpr uint32_t c_shadowMapResolution = 2048;

    constexpr int c_maxNumberOfActiveReflectionProbes = 8;
}