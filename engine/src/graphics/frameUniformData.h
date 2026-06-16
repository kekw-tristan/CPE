#pragma once 

struct sFrameUniformData 
{
    float viewMatrix[16];
    float projMatrix[16];
    float viewProj  [16];

    float cameraPosition[4];
    float cameraDirection[4];
    
    float viewportSize[4];
    float clipPlanes[4];
};