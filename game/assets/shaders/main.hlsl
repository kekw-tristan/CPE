cbuffer FrameUniformBuffer : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProj;

    float4 cameraPosition;
    float4 cameraDirection;

    float4 viewportSize;
    float4 clipPlanes;
};

struct ObjectPushConstants
{
    row_major float4x4 worldMatrix;
};

[[vk::push_constant]]
ObjectPushConstants objectData;

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPosition = float4(input.position, 1.0f);

    float4 worldPosition = mul(localPosition, objectData.worldMatrix);

    output.position = mul(viewProj, worldPosition);

    output.texCoord = input.texCoord;
    output.color    = input.color;

    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    return input.color;
}