[[vk::binding(0, 0)]]
TextureCube<float4> captureMap;

[[vk::binding(1, 0)]]
SamplerState captureSampler;


// -----------------------------------------------------------------------------------------------------------------------------
// Push constants
// -----------------------------------------------------------------------------------------------------------------------------

struct ReflectionProbePrefilterPushConstants
{
    uint faceIndex;
    float roughness;
    uint sampleCount;
    float captureResolution;
};

[[vk::push_constant]]
ReflectionProbePrefilterPushConstants pushConstants;


// -----------------------------------------------------------------------------------------------------------------------------

static const float PI = 3.14159265359f;
static const float CAPTURE_RESOLUTION = 256.0f;


// -----------------------------------------------------------------------------------------------------------------------------
// Vertex shader
// -----------------------------------------------------------------------------------------------------------------------------

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};


VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    float2 position;

    position.x = vertexID == 2 ? 3.0f : -1.0f;
    position.y = vertexID == 1 ? 3.0f : -1.0f;

    output.position = float4(position, 0.0f, 1.0f);

    output.texCoord = position * 0.5f + 0.5f;

    return output;
}


// -----------------------------------------------------------------------------------------------------------------------------
// Math
// -----------------------------------------------------------------------------------------------------------------------------

float RadicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);

    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10f;
}


// -----------------------------------------------------------------------------------------------------------------------------

float2 Hammersley(uint index, uint sampleCount)
{
    return float2(float(index) / float(sampleCount), RadicalInverseVdC(index));
}


// -----------------------------------------------------------------------------------------------------------------------------

float3 SafeNormalize(float3 value)
{
    float lengthSquared = dot(value, value);

    if (lengthSquared <= 0.0000001f)
        return float3(0.0f, 0.0f, 1.0f);

    return value * rsqrt(lengthSquared);
}


// -----------------------------------------------------------------------------------------------------------------------------
// GGX importance sampling
// -----------------------------------------------------------------------------------------------------------------------------

float3 ImportanceSampleGGX(float2 xi, float3 normal, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;

    float phi = 2.0f * PI * xi.x;

    float cosTheta = sqrt((1.0f - xi.y) / max(1.0f + (alphaSquared - 1.0f) * xi.y, 0.000001f));
    float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));

    float3 halfVector;

    halfVector.x = cos(phi) * sinTheta;
    halfVector.y = sin(phi) * sinTheta;
    halfVector.z = cosTheta;

    float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);

    float3 tangent = SafeNormalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    float3 sampleVector = tangent * halfVector.x + bitangent * halfVector.y + normal * halfVector.z;

    return SafeNormalize(sampleVector);
}

// -----------------------------------------------------------------------------------------------------------------------------

float DistributionGGX(float3 normal, float3 halfVector, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;

    float NdotH = saturate(dot(normal, halfVector));
    float NdotH2 = NdotH * NdotH;

    float denominator = NdotH2 * (alphaSquared - 1.0f) + 1.0f;

    return alphaSquared / max(PI * denominator * denominator, 0.000001f);
}

// -----------------------------------------------------------------------------------------------------------------------------
// Convert current output cubemap pixel into a world-space cubemap direction.
//
// This follows the same face convention as our captured Vulkan cubemap.
// -----------------------------------------------------------------------------------------------------------------------------

float3 GetCubeDirection(uint faceIndex, float2 uv)
{
    float u = uv.x * 2.0f - 1.0f;
    float v = uv.y * 2.0f - 1.0f;

    float3 direction;

    switch (faceIndex)
    {
        case 0:
            direction = float3(1.0f, -v, -u);
            break;

        case 1:
            direction = float3(-1.0f, -v, u);
            break;

        case 2:
            direction = float3(u, 1.0f, v);
            break;

        case 3:
            direction = float3(u, -1.0f, -v);
            break;

        case 4:
            direction = float3(u, -v, 1.0f);
            break;

        case 5:
            direction = float3(-u, -v, -1.0f);
            break;

        default:
            direction = float3(0.0f, 1.0f, 0.0f);
            break;
    }

    return SafeNormalize(direction);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Fragment shader
// -----------------------------------------------------------------------------------------------------------------------------

float4 PSMain(VSOutput input) : SV_Target
{
    float roughness = saturate(pushConstants.roughness);

    float3 normal = GetCubeDirection(pushConstants.faceIndex, input.texCoord);
    float3 viewDirection = normal;

    if (roughness <= 0.0001f)
    {
        float3 color = captureMap.SampleLevel(captureSampler, normal, 0.0f).rgb;

        return float4(color, 1.0f);
    }

    float3 prefilteredColor = float3(0.0f, 0.0f, 0.0f);

    float totalWeight = 0.0f;

    uint sampleCount = max(pushConstants.sampleCount, 1u);

    float captureResolution = max(pushConstants.captureResolution, 1.0f);

    float texelSolidAngle = 4.0f * PI / (6.0f * captureResolution * captureResolution);

    float maxSourceMip = log2(captureResolution);

    for (uint sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        float2 xi = Hammersley(sampleIndex, sampleCount);

        float3 halfVector = ImportanceSampleGGX(xi, normal, roughness);

        float HdotV = saturate(dot(halfVector, viewDirection));

        float3 lightDirection = SafeNormalize(2.0f * HdotV * halfVector - viewDirection);

        float NdotL = saturate(dot(normal, lightDirection));

        if (NdotL > 0.0f)
        {
            float NdotH = saturate(dot(normal, halfVector));

            float distribution = DistributionGGX(normal, halfVector, roughness);

            float pdf = distribution * NdotH / max(4.0f * HdotV, 0.000001f);
            pdf = max(pdf, 0.000001f);

            float sampleSolidAngle = 1.0f / (float(sampleCount) * pdf);

            float sourceMip = 0.5f * log2(sampleSolidAngle / texelSolidAngle);
            sourceMip = clamp(sourceMip, 0.0f, maxSourceMip);

            float3 sampleColor = captureMap.SampleLevel(captureSampler, lightDirection, sourceMip).rgb;

            prefilteredColor += sampleColor * NdotL;
            totalWeight += NdotL;
        }
    }

    if (totalWeight > 0.0f)
    {
        prefilteredColor /= totalWeight;
    }

    return float4(max(prefilteredColor, 0.0f), 1.0f);
}