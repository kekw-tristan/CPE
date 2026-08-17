#pragma once 

#include "graphics/vulkan/reflectionProbe.h"
#include "graphics/gfxConfig.h"



struct sFrameUniformData 
{
    float viewMatrix[16];
    float projMatrix[16];
    float viewProj  [16];

    float cameraPosition[4];
    float cameraDirection[4];
    
    float viewportSize[4];
    float clipPlanes[4];

    uint32_t lightCount;
    uint32_t materialCount;
    uint32_t reflectionProbeCount;
    uint32_t padding1;

    Engine::GFX::sReflectionProbeGPU reflectionProbes[Engine::GFX::c_maxNumberOfReflectionProbes];
};