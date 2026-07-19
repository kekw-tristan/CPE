[[vk::binding(0, 0)]]
cbuffer FrameUniformBuffer
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProj;

    float4 cameraPosition;
    float4 cameraDirection;

    float4 viewportSize;
    float4 clipPlanes;
};


struct InstanceData
{
    row_major float4x4 worldMatrix;
    float4 color;
};


[[vk::binding(1, 0)]]
StructuredBuffer<InstanceData> instances;


struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
};


struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR0;
};


VSOutput VSMain(
    VSInput input,
    uint instanceID : SV_InstanceID)
{
    VSOutput output;

    InstanceData instance = instances[instanceID];

    float4 localPosition = float4(input.position, 1.0f);

    float4 worldPosition =
        mul(localPosition, instance.worldMatrix);

    output.position =
        mul(viewProj, worldPosition);

    output.texCoord = input.texCoord;

    // Instanzfarbe statt Mesh-Farbe
    output.color = instance.color;

    return output;
}


float4 PSMain(VSOutput input) : SV_Target
{
    return input.color;
}