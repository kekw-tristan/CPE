[[vk::binding(0, 0)]]
cbuffer FrameUniformBuffer
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProj;
};

struct VSInput
{
    [[vk::location(0)]] float4 positionSize : POSITION0;
    [[vk::location(1)]] float4 color        : COLOR0;
    [[vk::location(2)]] float4 normalMode   : TEXCOORD0;
    [[vk::location(3)]] float4 rotationAge  : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    nointerpolation float4 color : COLOR0;
    nointerpolation float2 modeAge : TEXCOORD1;
};

VSOutput VSMain(VSInput _input, uint _vertexId : SV_VertexID)
{
    const float2 corners[6] =
    {
        float2(-1.0, -1.0), float2(1.0, -1.0), float2(1.0, 1.0),
        float2(-1.0, -1.0), float2(1.0, 1.0), float2(-1.0, 1.0)
    };
    VSOutput output;
    output.uv       = corners[_vertexId];
    output.color    = _input.color;
    output.modeAge  = float2(_input.normalMode.w, _input.rotationAge.y);
    
    float2 offset = output.uv * _input.positionSize.w;
    
    if (_input.normalMode.w > 0.5)
    {
        // Preserve the gameplay footprint in X/Z while following the sampled slope.
        float3 normal   = _input.normalMode.xyz;
        float  height   = -dot(normal.xz, offset) / max(normal.y, 0.1);
        float3 position = _input.positionSize.xyz + float3(offset.x, height, offset.y);
        
        output.position = mul(viewProj, float4(position, 1.0));
    }
    else
    {
        float sine;
        float cosine;
        
        sincos(_input.rotationAge.x, sine, cosine);
        offset = float2(offset.x * cosine - offset.y * sine, offset.x * sine + offset.y * cosine);
        
        float4 position = mul(viewMatrix, float4(_input.positionSize.xyz, 1.0));
        
        position.xy += offset;
        output.position = mul(projMatrix, position);
    }
    return output;
}

float4 PSMain(VSOutput _input) : SV_Target
{
    float radius = length(_input.uv);
    float edge   = 1.0 - smoothstep(_input.modeAge.x > 0.5 ? 0.35 : 0.15, 1.0, radius);
    float motion = sin(_input.uv.x * 7.0 + _input.modeAge.y * 1.6) * sin(_input.uv.y * 6.0 - _input.modeAge.y * 1.2);
    float alpha  = _input.color.a * edge * (0.86 + motion * 0.14);
    
    clip(alpha - 0.002);
    
    return float4(_input.color.rgb * alpha, alpha);
}
