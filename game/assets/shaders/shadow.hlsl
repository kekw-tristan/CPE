struct InstanceData
{
    row_major float4x4 worldMatrix;
    float4 color;

    int materialIndex;
    int padding1;
    int padding2;
    int padding3;
};

struct ShadowData
{
    row_major float4x4 viewProjection[6];

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

float4 VSMain(float3 position : POSITION, uint instanceID : SV_InstanceID) : SV_Position
{
    InstanceData instance = instances[instanceID];

    float4 worldPosition = mul(float4(position, 1.0f), instance.worldMatrix);

    return mul(worldPosition, shadows[pushConstants.shadowIndex].viewProjection[pushConstants.matrixIndex]);
}