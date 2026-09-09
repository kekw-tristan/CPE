#pragma once 

#include <cstdint>
#include <array>

namespace Engine::GFX
{
    struct sEnvironmentSettings
    {
        std::array<float, 3> groundColor = { 0.030f, 0.045f, 0.035f };
        std::array<float, 3> horizonColor = { 0.22f, 0.34f, 0.50f };
        std::array<float, 3> zenithColor = { 0.018f, 0.028f, 0.045f };
        std::array<float, 3> keyDirection = { -0.55f, 0.40f, -0.72f };
        std::array<float, 3> keyRadiance = { 0.68f, 0.82f, 1.0f };
        float keyExponent = 1200.0f;
    };

    static constexpr uint32_t c_maxNumberOfFrames       = 2; 

    static constexpr uint32_t c_maxNumberOfInstances    = 100000; 
    static constexpr uint32_t c_maxNumberOfLights       = 1000;
    static constexpr uint32_t c_maxNumberOfActiveLights = 48;
    static constexpr uint32_t c_maxNumberOfMaterials    = 10000;

    static constexpr uint32_t c_maxShadowLayers = 16;

    static constexpr uint32_t c_directionalCascadeCount = 2;

    static constexpr float c_directionalCascadeSplits[c_directionalCascadeCount] =
    {
        40.f,
        300.f,
    };

    static constexpr uint32_t c_shadowMapResolution = 4096;

    constexpr int c_maxNumberOfActiveReflectionProbes = 8;
    constexpr int c_maxNumberOfReflectionProbes       = 96;
}
