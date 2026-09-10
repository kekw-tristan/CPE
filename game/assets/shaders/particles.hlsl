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
    [[vk::location(4)]] float4 surfaceClip  : TEXCOORD2;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    nointerpolation float4 color : COLOR0;
    nointerpolation float2 modeAge : TEXCOORD1;
    float2 worldXZ : TEXCOORD2;
    nointerpolation float3 areaClip : TEXCOORD3;
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
    output.worldXZ = _input.positionSize.xz;
    output.areaClip = _input.surfaceClip.xyz;
    
    float2 offset = output.uv * _input.positionSize.w;
    
    if (_input.normalMode.w > 0.5)
    {
        // Preserve the gameplay footprint in X/Z while following the sampled slope.
        float3 normal   = _input.normalMode.xyz;
        float  height   = -dot(normal.xz, offset) / max(normal.y, 0.1);
        float3 position = _input.positionSize.xyz + float3(offset.x, height, offset.y);
        
        output.worldXZ = position.xz;
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

float Hash(float2 _point)
{
    return frac(sin(dot(_point, float2(127.1, 311.7))) * 43758.5453);
}

float Noise(float2 _point)
{
    float2 cell = floor(_point);
    float2 t = frac(_point);
    t = t * t * (3.0 - 2.0 * t);
    return lerp(lerp(Hash(cell), Hash(cell + float2(1, 0)), t.x),
        lerp(Hash(cell + float2(0, 1)), Hash(cell + float2(1, 1)), t.x), t.y);
}

float4 PSMain(VSOutput _input) : SV_Target
{
    float age = _input.modeAge.y;
    float mode = _input.modeAge.x;
    float3 color = _input.color.rgb;
    float alpha;
    if (mode > 1.5)
    {
        // World-space noise crosses tile edges without a visible grid or repeated circles.
        float2 p = _input.worldXZ;
        float distanceToEdge = _input.areaClip.z - length(p - _input.areaClip.xy);
        clip(distanceToEdge);
        float edge = smoothstep(0.0, 0.22, distanceToEdge);
        float drift = Noise(p * 0.85 + float2(age * 0.12, -age * 0.09));
        float swirls = Noise(p * 2.1 + float2(drift * 2.0, age * 0.18));
        float veins = pow(saturate(1.0 - abs(swirls - 0.5) * 8.0), 3.0);
        float rim = (1.0 - smoothstep(0.12, 0.42, distanceToEdge)) * edge;
        color = lerp(color * 0.055, color * 0.28, drift) + color * (veins * 0.22 + rim * 0.35);
        alpha = _input.color.a * edge * (0.82 + 0.18 * swirls);
    }
    else if (mode < -1.5)
    {
        float radius = length(_input.uv);
        float shell = exp(-pow((radius - 0.72) * 15.0, 2.0));
        float glint = exp(-dot(_input.uv - float2(-0.24, -0.32), _input.uv - float2(-0.24, -0.32)) * 90.0);
        alpha = _input.color.a * (shell * 0.65 + glint * 0.8);
        color = lerp(color, float3(0.7, 1.0, 0.25), glint);
    }
    else if (mode < -0.5)
    {
        float2 uv = _input.uv;
        float cloud = Noise(uv * 2.8 + float2(age * 0.16, -age * 0.12));
        float radius = length(uv * float2(0.85, 1.0));
        float edge = 1.0 - smoothstep(0.25, 0.95, radius + (cloud - 0.5) * 0.25);
        alpha = _input.color.a * edge * (0.4 + cloud * 0.6);
        color *= 0.35 + cloud * 0.35;
    }
    else
    {
        float radius = length(_input.uv);
        float edge = 1.0 - smoothstep(0.15, 1.0, radius);
        alpha = _input.color.a * edge;
    }
    clip(alpha - 0.002);
    return float4(color * alpha, alpha);
}
