#pragma once

#include <cstdint>

namespace Engine::GFX
{
    struct sReflectionProbePrefilterPushConstants
    {
        uint32_t faceIndex          = 0;
        float    roughness          = 0.0f;
        uint32_t sampleCount        = 128;
        float    captureResolution  = 256.0f;
    };

    static_assert(sizeof(sReflectionProbePrefilterPushConstants) == 16);
}