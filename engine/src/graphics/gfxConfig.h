#pragma once 

#include <cstdint>
#include <array>

namespace Engine::GFX
{
    struct sEnvironmentSettings
    {
        std::array<float, 3> groundColor  = { 0.030f, 0.045f, 0.035f };
        std::array<float, 3> horizonColor = { 0.22f, 0.34f, 0.50f };
        std::array<float, 3> zenithColor  = { 0.018f, 0.028f, 0.045f };
        std::array<float, 3> keyDirection = { -0.55f, 0.40f, -0.72f };
        std::array<float, 3> keyRadiance  = { 0.68f, 0.82f, 1.0f };

        float keyExponent = 1200.0f;
    };

    // Optional local atmosphere, packed as seven float4 values for the frame buffer.
    // A zero blend distance disables it. Bounds enclose an elliptical cylinder;
    // the shaft follows the vertical Y axis with a shared top/bottom XZ center.
    struct sLocalAtmosphereSettings
    {
        std::array<float, 4> boundsMinBlend{}; // XYZ minimum, W boundary blend distance.
        std::array<float, 4> boundsMaxAmbient = { 0.0f, 0.0f, 0.0f, 1.0f }; // XYZ maximum, W ambient multiplier.
        std::array<float, 4> fogColorDensity{}; // RGB linear color, W extinction per world unit.
        std::array<float, 4> fogHeightStart{}; // Base height, start distance, height falloff, height density.
        std::array<float, 4> shaftTopRadius{}; // XYZ top center, W top radius.
        std::array<float, 4> shaftBottomRadius{}; // XYZ bottom center, W bottom radius.
        std::array<float, 4> shaftColorDensity{}; // RGB linear radiance, W extinction per world unit.
    };

    static_assert(sizeof(sLocalAtmosphereSettings) == 112);

    static constexpr uint32_t c_maxNumberOfFrames       = 2; 

    static constexpr uint32_t c_maxNumberOfInstances    = 100000; 
    static constexpr uint32_t c_maxNumberOfLights       = 1000;
    static constexpr uint32_t c_maxNumberOfActiveLights = 24;
    static constexpr uint32_t c_maxNumberOfMaterials    = 10000;

    static constexpr uint32_t c_maxShadowLayers = 16;

    static constexpr uint32_t c_directionalCascadeCount = 2;

    static constexpr float c_directionalCascadeSplits[c_directionalCascadeCount] =
    {
        40.f,
        300.f,
    };

    static constexpr uint32_t c_shadowMapResolution = 4096;

    constexpr int c_maxNumberOfActiveReflectionProbes = 4;
    constexpr int c_maxNumberOfReflectionProbes       = 256;
}
