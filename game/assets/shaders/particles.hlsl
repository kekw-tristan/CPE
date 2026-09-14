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
        // Stationary triangular facets form a dry spore bed across terrain tile seams.
        float2 p = _input.worldXZ;
        float distanceToEdge = _input.areaClip.z - length(p - _input.areaClip.xy);
        clip(distanceToEdge);
        float edge = smoothstep(0.0, 0.10, distanceToEdge);
        float2 lattice = float2(p.x - p.y * 0.57735, p.y * 1.15470) * 1.8;
        float2 cell = floor(lattice);
        float2 local = frac(lattice);
        float triangleIndex = step(1.0, local.x + local.y);
        float facet = Hash(cell + triangleIndex * float2(17.0, 31.0));
        float seamDistance = min(min(min(local.x, local.y), min(1.0 - local.x, 1.0 - local.y)),
            abs(local.x + local.y - 1.0) * 0.7071);
        float mycelium = 1.0 - smoothstep(0.015, 0.045, seamDistance);
        float rim = 1.0 - smoothstep(0.06, 0.20, distanceToEdge);
        float pulse = 0.95 + 0.05 * sin(age * 2.5);
        color *= (0.22 + 0.18 * facet + 0.12 * mycelium + 0.25 * rim) * pulse;
        alpha = _input.color.a * edge;
    }
    else if (mode < -3.5)
    {
        // Upright, beveled skull silhouette with inset eyes and three square teeth.
        float2 p = _input.uv;
        float2 head = abs(p - float2(0.0, 0.18));
        float headDistance = max(max(head.x - 0.64, head.y - 0.57), head.x + head.y - 1.0);
        float jawDistance = max(abs(p.x) - 0.40, abs(p.y + 0.43) - 0.27);
        float silhouette = min(headDistance, jawDistance);
        float aa = max(fwidth(silhouette), 0.008);
        float mask = 1.0 - smoothstep(-aa, aa, silhouette);
        float2 eye = abs(float2(abs(p.x) - 0.28, p.y - 0.15));
        float eyes = 1.0 - smoothstep(-aa, aa, max(eye.x - 0.17, eye.y - 0.16));
        float nose = 1.0 - smoothstep(-aa, aa, abs(p.x) + abs(p.y + 0.14) - 0.12);
        float teeth = step(p.y, -0.40) * (1.0 - smoothstep(0.025, 0.025 + aa, abs(abs(p.x) - 0.135)));
        color *= lerp(0.08, 1.0, 1.0 - max(eyes, nose));
        alpha = _input.color.a * mask * (1.0 - teeth);
    }
    else if (mode < -2.5)
    {
        float2 p = _input.uv;
        float shape = abs(p.x) + abs(p.y) * 0.75 - 0.72;
        float aa = max(fwidth(shape), 0.015);
        alpha = _input.color.a * (1.0 - smoothstep(-aa, aa, shape));
        color *= p.x + p.y * 0.4 > 0.0 ? 0.65 : 1.0;
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
