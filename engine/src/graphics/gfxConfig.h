#pragma once 

#include <cstdint>

namespace Engine::GFX
{
    static constexpr uint32_t c_maxNumberOfFrames       = 2; 

    static constexpr uint32_t c_maxNumberOfInstances    = 10000; 
    static constexpr uint32_t c_maxNumberOfLights       = 1000;
    static constexpr uint32_t c_maxNumberOfMaterials    = 1000;

    static constexpr uint32_t c_maxShadowLayers = 16;

    static constexpr uint32_t c_directionalCascadeCount = 4;

    static constexpr float c_directionalCascadeSplits[c_directionalCascadeCount] =
    {
        30.0f,
        70.0f,
        150.0f,
        300.f
    };

    static constexpr uint32_t c_shadowMapResolution = 4096;

    constexpr int c_maxNumberOfActiveReflectionProbes = 8;
}