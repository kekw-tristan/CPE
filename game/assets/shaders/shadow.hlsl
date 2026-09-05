static const int INSTANCE_FLAG_TERRAIN = 1;

struct InstanceData
{
    row_major float4x4 worldMatrix;
    float4 color;

    int materialIndex;
    int instanceFlags;
    int padding2;
    int padding3;
};

struct ShadowData
{
    row_major float4x4 viewProjection[6];

    float4 cascadeSplits;

    uint lightIndex;
    uint firstLayer;
    uint matrixCount;
    uint padding;
};

struct ShadowPushConstants
{
    uint shadowIndex;
    uint matrixIndex;
};

[[vk::binding(1, 0)]]
StructuredBuffer<InstanceData> instances;

[[vk::binding(4, 0)]]
StructuredBuffer<ShadowData> shadows;

[[vk::push_constant]]
ConstantBuffer<ShadowPushConstants> pushConstants;

#include "../../src/world/terrainHeight.h"

float GetTerrainHeight(float2 worldPosition)
{
    return GetTerrainHeight(worldPosition.x, worldPosition.y);
}

float4 VSMain(float3 position : POSITION, uint instanceID : SV_InstanceID) : SV_Position
{
    InstanceData instance = instances[instanceID];

    float4 worldPosition = mul(float4(position, 1.0f), instance.worldMatrix);

    if ((instance.instanceFlags & INSTANCE_FLAG_TERRAIN) != 0)
    {
        worldPosition.y += GetTerrainHeight(worldPosition.xz);
    }

    return mul(worldPosition, shadows[pushConstants.shadowIndex].viewProjection[pushConstants.matrixIndex]);
}