[[vk::binding(0, 0)]]
cbuffer FrameUniformBuffer
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProj;
};

struct VSInput
{
    [[vk::location(0)]] float4 positionWidth    : POSITION0;
    [[vk::location(1)]] float4 heightFill       : TEXCOORD0;
    [[vk::location(2)]] float4 color            : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    nointerpolation float2 size     : TEXCOORD1;
    nointerpolation float  fill     : TEXCOORD2;
    nointerpolation float  tier     : TEXCOORD3;
    nointerpolation float4 color    : COLOR0;
};

VSOutput VSMain(VSInput _input, uint _vertexId : SV_VertexID)
{
    const float2 corners[6] =
    {
        float2(0.0, 0.0), float2(1.0, 0.0), float2(1.0, 1.0),
        float2(0.0, 0.0), float2(1.0, 1.0), float2(0.0, 1.0)
    };

    VSOutput output;
    output.uv       = corners[_vertexId];
    output.size     = float2(_input.positionWidth.w, _input.heightFill.x);
    output.fill     = saturate(_input.heightFill.y);
    output.tier     = _input.heightFill.z;
    output.color    = _input.color;

    float4 viewPosition = mul(viewMatrix, float4(_input.positionWidth.xyz, 1.0));

    viewPosition.xy += (output.uv - 0.5) * output.size;
    output.position = mul(projMatrix, viewPosition);

    return output;
}

float4 PSMain(VSOutput _input) : SV_Target
{
    const float borderWidth = 0.022;
    const float2 border     = min(borderWidth / max(_input.size, 0.0001), 0.25);
    const float2 edge       = min(_input.uv, 1.0 - _input.uv);

    if (any(edge < border))
    {
        const float3 borderColor = _input.tier < 0.5 ? float3(1.0, 1.0, 1.0)
            : _input.tier < 1.5 ? float3(0.03, 0.38, 1.0)
            : _input.tier < 2.5 ? float3(1.0, 0.55, 0.02)
            : float3(0.92, 0.08, 1.0);
        return float4(borderColor, 1.0);
    }

    const float interiorX = (_input.uv.x - border.x) / (1.0 - 2.0 * border.x);

    if (interiorX < _input.fill)
    {
        return float4(_input.color.rgb, 1.0);
    }

    return float4(0.065, 0.065, 0.075, 1.0);
}
