static const uint MAX_REFLECTION_PROBES = 8;
[[vk::binding(14, 0)]] Texture2D<float4> occlusionGeometry;
[[vk::binding(15, 0)]] Texture2D<float4> occlusionRaw;
[[vk::binding(16, 0)]] Texture2D<float4> occlusionFiltered;
static const int  INSTANCE_FLAG_TERRAIN = 1;
static const int  INSTANCE_FLAG_SKY = 2;
static const int  INSTANCE_FLAG_PRESERVE_AT_DISTANCE = 4;
static const int  INSTANCE_FLAG_CRYSTAL = 8;

struct ReflectionProbeData
{
    float4 positionMaxMip;
    float4 boxMinBlendDistance;
    float4 boxMax;
};


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

    uint lightCount;
    uint materialCount;
    uint reflectionProbeCount;
    uint activeLightCount;

    ReflectionProbeData reflectionProbes[MAX_REFLECTION_PROBES];
};


struct InstanceData
{
    row_major float4x4 worldMatrix;

    float4 color;

    int materialIndex;
    int instanceFlags;
    int padding2;
    int padding3;
};


[[vk::binding(1, 0)]]
StructuredBuffer<InstanceData> instances;


struct LightData
{
    float4 positionRadius;
    float4 directionType;
    float4 colorIntensity;
    float4 spotData;

    int shadowIndex;
    int padding0;
    int padding1;
    int padding2;
};


[[vk::binding(2, 0)]]
StructuredBuffer<LightData> lights;

[[vk::binding(13, 0)]]
StructuredBuffer<uint> activeLightIndices;


struct MaterialData
{
    // rgb = albedo
    // a   = ambient strength
    float4 albedo;

    // x = roughness
    // y = metallic
    // z = light wrap
    // w = shape contrast
    float4 properties;

    // rgb = emissive color
    // a   = emissive strength
    float4 emissiveColor;
};


[[vk::binding(3, 0)]]
StructuredBuffer<MaterialData> materials;


struct ShadowData
{
    row_major float4x4 viewProjection[6];

    float4 cascadeSplits;

    uint lightIndex;
    uint firstLayer;
    uint matrixCount;
    uint padding;
};


[[vk::binding(4, 0)]]
StructuredBuffer<ShadowData> shadows;


[[vk::binding(5, 0)]]
Texture2DArray<float> shadowMap;


[[vk::binding(6, 0)]]
SamplerState shadowSampler;


[[vk::binding(7, 0)]]
TextureCube<float4> environmentMap;


[[vk::binding(8, 0)]]
SamplerState environmentSampler;


[[vk::binding(9, 0)]]
Texture2D<float2> brdfLUT;


[[vk::binding(10, 0)]]
SamplerState brdfSampler;


[[vk::binding(11, 0)]]
TextureCube<float4> irradianceMap;


[[vk::binding(12, 0)]]
TextureCube<float4> reflectionProbeMaps[MAX_REFLECTION_PROBES];


static const float PI = 3.14159265359f;

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;

static const uint REFLECTION_PROBE_PROJECTION_INFINITE = 0;
static const uint REFLECTION_PROBE_PROJECTION_BOX = 1;

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};


struct VSOutput
{
    float4 position : SV_Position;

    float3 worldPosition    : POSITION0;
    float3 worldNormal      : NORMAL0;

    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR0;

    nointerpolation int materialIndex : MATERIAL_INDEX;
    nointerpolation uint terrain : TEXCOORD7;
    nointerpolation uint sky : TEXCOORD8;
    nointerpolation uint preserveAtDistance : TEXCOORD9;
    nointerpolation uint crystal : TEXCOORD10;
    nointerpolation uint surfaceFlags : TEXCOORD11;
};

struct NormalDepthVSOutput
{
    float4 position : SV_Position;

    float3 worldPosition : POSITION0;
    float3 worldNormal : NORMAL0;

    nointerpolation uint terrain : TEXCOORD7;
    nointerpolation uint sky : TEXCOORD8;
    nointerpolation uint preserveAtDistance : TEXCOORD9;
};


// -----------------------------------------------------------------------------------------------------------------------------
// Math
// -----------------------------------------------------------------------------------------------------------------------------

float3 SafeNormalize(float3 value)
{
    float lengthSquared = dot(value, value);

    if (lengthSquared <= 0.0000001f)
        return float3(0.0f, 0.0f, 0.0f);

    return value * rsqrt(lengthSquared);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Vertex Shader
// -----------------------------------------------------------------------------------------------------------------------------

#include "../../src/world/terrainHeight.h"
#include "../../src/world/forestAtmosphere.h"
#include "nightSky.hlsl"

float GetTerrainHeight(float2 worldPosition)
{
    return GetTerrainHeight(worldPosition.x, worldPosition.y);
}

float3 GetTerrainNormal(float2 worldPosition)
{
    const float sampleDistance = 0.5f;

    float heightLeft = GetTerrainHeight(worldPosition + float2(-sampleDistance, 0.0f));
    float heightRight = GetTerrainHeight(worldPosition + float2(sampleDistance, 0.0f));

    float heightBack = GetTerrainHeight(worldPosition + float2(0.0f, -sampleDistance));
    float heightFront = GetTerrainHeight(worldPosition + float2(0.0f, sampleDistance));

    float3 normal = float3(
        heightLeft - heightRight,
        2.0f * sampleDistance,
        heightBack - heightFront
    );

    return SafeNormalize(normal);
}

VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output = (VSOutput)0;

    InstanceData instance = instances[instanceID];
    if ((instance.instanceFlags & INSTANCE_FLAG_SKY) != 0)
    {
        output.sky = 1;
        output.worldPosition = input.position;
        output.position = mul(viewProj, float4(cameraPosition.xyz + input.position * 1000.0f, 1.0f));
        output.position.z = output.position.w * 0.999999f;
        return output;
    }


    float4 worldPosition = mul(float4(input.position, 1.0f), instance.worldMatrix);
    
    if ((instance.instanceFlags & INSTANCE_FLAG_TERRAIN) != 0)
    {
        worldPosition.y += GetTerrainHeight(worldPosition.xz);

        output.worldNormal = GetTerrainNormal(worldPosition.xz);
    }
    else
    {
        float3x3 normalMatrix = (float3x3) instance.worldMatrix;

        output.worldNormal = SafeNormalize(mul(input.normal, normalMatrix));
    }

    output.position = mul(viewProj, worldPosition);
    output.worldPosition = worldPosition.xyz;

    output.terrain = (instance.instanceFlags & INSTANCE_FLAG_TERRAIN) != 0 ? 1u : 0u;
    output.preserveAtDistance = (instance.instanceFlags & INSTANCE_FLAG_PRESERVE_AT_DISTANCE) != 0 ? 1u : 0u;
    output.crystal = (instance.instanceFlags & INSTANCE_FLAG_CRYSTAL) != 0 ? 1u : 0u;
    output.texCoord = input.texCoord;
    output.color = instance.color;

    output.materialIndex = instance.materialIndex;
    output.surfaceFlags = (uint)instance.instanceFlags;

    return output;
}

NormalDepthVSOutput VSNormalDepth(VSInput input, uint instanceID : SV_InstanceID)
{
    NormalDepthVSOutput output = (NormalDepthVSOutput) 0;

    InstanceData instance = instances[instanceID];

    if ((instance.instanceFlags & INSTANCE_FLAG_SKY) != 0)
    {
        output.sky = 1;
        output.worldPosition = input.position;
        output.position = mul(viewProj, float4(cameraPosition.xyz + input.position * 1000.0f, 1.0f));
        output.position.z = output.position.w * 0.999999f;

        return output;
    }

    float4 worldPosition = mul(float4(input.position, 1.0f), instance.worldMatrix);

    if ((instance.instanceFlags & INSTANCE_FLAG_TERRAIN) != 0)
    {
        worldPosition.y += GetTerrainHeight(worldPosition.xz);
        output.worldNormal = GetTerrainNormal(worldPosition.xz);
    }
    else
    {
        float3x3 normalMatrix = (float3x3) instance.worldMatrix;
        output.worldNormal = SafeNormalize(mul(input.normal, normalMatrix));
    }

    output.position = mul(viewProj, worldPosition);
    output.worldPosition = worldPosition.xyz;

    output.terrain = (instance.instanceFlags & INSTANCE_FLAG_TERRAIN) != 0 ? 1u : 0u;
    output.sky = 0u;
    output.preserveAtDistance = (instance.instanceFlags & INSTANCE_FLAG_PRESERVE_AT_DISTANCE) != 0 ? 1u : 0u;

    return output;
}


// -----------------------------------------------------------------------------------------------------------------------------
// GGX
// -----------------------------------------------------------------------------------------------------------------------------

float DistributionGGX(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;

    float denominator = NdotH * NdotH * (alphaSquared - 1.0f) + 1.0f;
    denominator = PI * denominator * denominator;

    return alphaSquared / max(denominator, 0.000001f);
}


// -----------------------------------------------------------------------------------------------------------------------------

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotV / max(NdotV * (1.0f - k) + k, 0.000001f);
}


// -----------------------------------------------------------------------------------------------------------------------------

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float geometryView = GeometrySchlickGGX(NdotV, roughness);
    float geometryLight = GeometrySchlickGGX(NdotL, roughness);

    return geometryView * geometryLight;
}


// -----------------------------------------------------------------------------------------------------------------------------

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    float factor = pow(1.0f - saturate(cosTheta), 5.0f);

    return F0 + (1.0f - F0) * factor;
}


// -----------------------------------------------------------------------------------------------------------------------------

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float factor = pow(1.0f - saturate(cosTheta), 5.0f);

    float3 roughnessFresnel = max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0);

    return F0 + (roughnessFresnel - F0) * factor;
}


// -----------------------------------------------------------------------------------------------------------------------------
// Shadow
// -----------------------------------------------------------------------------------------------------------------------------

float CalculateShadow(float3 worldPosition, float3 normal, float3 lightDirection, uint shadowIndex, uint matrixIndex)
{
    ShadowData shadowData = shadows[shadowIndex];

    float NdotL = saturate(dot(normal, lightDirection));

    float normalBias = 0.02f * (1.0f - NdotL);

    float3 biasedWorldPosition = worldPosition + normal * normalBias;

    float4 lightSpacePosition = mul(float4(biasedWorldPosition, 1.0f), shadowData.viewProjection[matrixIndex]);

    if (lightSpacePosition.w <= 0.0f)
        return 0.0f;

    float3 projectedCoords = lightSpacePosition.xyz / lightSpacePosition.w;
    float2 shadowUV = projectedCoords.xy * 0.5f + 0.5f;
    float currentDepth = projectedCoords.z;

    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f)
        return 0.0f;

    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 0.0f;

    uint layer = shadowData.firstLayer + matrixIndex;

    float bias = max(0.00075f * (1.0f - NdotL), 0.000075f);
    float shadow = 0.0f;

    uint width;
    uint height;
    uint layers;

    shadowMap.GetDimensions(width, height, layers);

    float2 texelSize = 1.0f / float2(width, height);

    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 sampleUV = shadowUV + float2(x, y) * texelSize;

            float closestDepth = shadowMap.Sample(shadowSampler, float3(sampleUV, float(layer))).r;

            shadow += currentDepth - bias > closestDepth ? 1.0f : 0.0f;
        }
    }

    return shadow / 9.0f;
}


// -----------------------------------------------------------------------------------------------------------------------------

uint GetPointShadowMatrixIndex(float3 worldPosition, float3 lightPosition)
{
    float3 direction = worldPosition - lightPosition;
    float3 absDirection = abs(direction);

    if (absDirection.x >= absDirection.y && absDirection.x >= absDirection.z)
    {
        return direction.x >= 0.0f ? 0 : 1;
    }

    if (absDirection.y >= absDirection.x && absDirection.y >= absDirection.z)
    {
        return direction.y >= 0.0f ? 2 : 3;
    }

    return direction.z >= 0.0f ? 4 : 5;
}


// -----------------------------------------------------------------------------------------------------------------------------

uint GetDirectionalCascadeIndex(float viewDepth, ShadowData shadowData)
{
    if (viewDepth <= shadowData.cascadeSplits.x)
        return 0;

    if (viewDepth <= shadowData.cascadeSplits.y)
        return 1;

    if (viewDepth <= shadowData.cascadeSplits.z)
        return 2;

    if (viewDepth <= shadowData.cascadeSplits.w)
        return 3;

    return shadowData.matrixCount;
}

// -----------------------------------------------------------------------------------------------------------------------------

float GetCascadeBlendFactor(float viewDepth, uint cascadeIndex, ShadowData shadowData)
{
    if (cascadeIndex >= shadowData.matrixCount - 1)
        return 0.0f;

    float cascadeNear = cascadeIndex == 0 ? 0.0f : shadowData.cascadeSplits[cascadeIndex - 1];
    float cascadeFar = shadowData.cascadeSplits[cascadeIndex];

    float cascadeRange = cascadeFar - cascadeNear;
    float blendRange = cascadeRange * 0.1f;

    float blendStart = cascadeFar - blendRange;

    return saturate((viewDepth - blendStart) / blendRange);
}


// -----------------------------------------------------------------------------------------------------------------------------

float GetCameraViewDepth(float3 worldPosition)
{
    return dot(worldPosition - cameraPosition.xyz, SafeNormalize(cameraDirection.xyz));
}


// -----------------------------------------------------------------------------------------------------------------------------
// Shape Lighting
// -----------------------------------------------------------------------------------------------------------------------------

float EvaluateShapeDiffuse(float NdotLRaw, float lightWrap, float shapeContrast)
{
    lightWrap = saturate(lightWrap);
    shapeContrast = max(shapeContrast, 0.05f);

    float wrappedNdotL = saturate((NdotLRaw + lightWrap) / (1.0f + lightWrap));

    return pow(wrappedNdotL, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Surface BRDF
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateSurfaceLight(
    float3 worldPosition,
    float3 normal,
    float3 lightDirection,
    float3 radiance,
    float3 albedo,
    float roughness,
    float metallic,
    float lightWrap,
    float shapeContrast)
{
    float3 viewDirection = SafeNormalize(cameraPosition.xyz - worldPosition);

    float NdotLRaw = dot(normal, lightDirection);
    float NdotL = saturate(NdotLRaw);
    float NdotV = saturate(dot(normal, viewDirection));

    float shapedNdotL = EvaluateShapeDiffuse(NdotLRaw, lightWrap, shapeContrast);

    if (shapedNdotL <= 0.0f && NdotL <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float3 halfVector = SafeNormalize(viewDirection + lightDirection);

    float NdotH = saturate(dot(normal, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));

    roughness = clamp(roughness, 0.045f, 1.0f);
    metallic = saturate(metallic);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float distribution = DistributionGGX(NdotH, roughness);
    float geometry = GeometrySmith(NdotV, NdotL, roughness);
    float3 fresnel = FresnelSchlick(VdotH, F0);

    float3 specularNumerator = distribution * geometry * fresnel;
    float specularDenominator = max(4.0f * NdotV * NdotL, 0.0001f);

    float3 specular = specularNumerator / specularDenominator;

    float3 kS = fresnel;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    float3 diffuse = kD * albedo / PI;

    float3 diffuseContribution = diffuse * radiance * shapedNdotL;
    float3 specularContribution = specular * radiance * NdotL;

    return diffuseContribution + specularContribution;
}


// -----------------------------------------------------------------------------------------------------------------------------
// Directional Light
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateDirectionalLight(
    float3 worldPosition,
    float3 normal,
    float3 albedo,
    LightData light,
    float roughness,
    float metallic,
    float lightWrap,
    float shapeContrast)
{
    float3 lightDirection = SafeNormalize(-light.directionType.xyz);
    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    float3 radiance = lightColor * lightIntensity;

    return EvaluateSurfaceLight(worldPosition, normal, lightDirection, radiance, albedo, roughness, metallic, lightWrap, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Point Light
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluatePointLight(
    float3 worldPosition,
    float3 normal,
    float3 albedo,
    LightData light,
    float roughness,
    float metallic,
    float lightWrap,
    float shapeContrast)
{
    float3 toLight = light.positionRadius.xyz - worldPosition;

    float distanceSquared = dot(toLight, toLight);
    float distanceToLight = sqrt(max(distanceSquared, 0.0001f));

    float radius = max(light.positionRadius.w, 0.0001f);

    if (distanceToLight >= radius)
        return float3(0.0f, 0.0f, 0.0f);

    float3 lightDirection = toLight / distanceToLight;

    float normalizedDistance = distanceToLight / radius;

    float rangeAttenuation = saturate(1.0f - normalizedDistance * normalizedDistance * normalizedDistance * normalizedDistance);
    rangeAttenuation *= rangeAttenuation;

    float distanceAttenuation = 1.0f / (1.0f + distanceSquared);

    float attenuation = rangeAttenuation * distanceAttenuation;

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    float3 radiance = lightColor * lightIntensity * attenuation;

    return EvaluateSurfaceLight(worldPosition, normal, lightDirection, radiance, albedo, roughness, metallic, lightWrap, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Spot Light
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateSpotLight(
    float3 worldPosition,
    float3 normal,
    float3 albedo,
    LightData light,
    float roughness,
    float metallic,
    float lightWrap,
    float shapeContrast)
{
    float3 toLight = light.positionRadius.xyz - worldPosition;

    float distanceSquared = dot(toLight, toLight);
    float distanceToLight = sqrt(max(distanceSquared, 0.0001f));

    float radius = max(light.positionRadius.w, 0.0001f);

    if (distanceToLight >= radius)
        return float3(0.0f, 0.0f, 0.0f);

    float3 lightDirection = toLight / distanceToLight;

    float3 spotDirection = SafeNormalize(light.directionType.xyz);

    float cosAngle = dot(-lightDirection, spotDirection);

    float innerCone = light.spotData.x;
    float outerCone = light.spotData.y;

    float spotFactor = smoothstep(outerCone, innerCone, cosAngle);

    if (spotFactor <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float normalizedDistance = distanceToLight / radius;

    float rangeAttenuation = saturate(1.0f - normalizedDistance * normalizedDistance * normalizedDistance * normalizedDistance);
    rangeAttenuation *= rangeAttenuation;

    float distanceAttenuation = 1.0f / (1.0f + distanceSquared);

    float attenuation = rangeAttenuation * distanceAttenuation * spotFactor;

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    float3 radiance = lightColor * lightIntensity * attenuation;

    return EvaluateSurfaceLight(worldPosition, normal, lightDirection, radiance, albedo, roughness, metallic, lightWrap, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Reflection Probe Box Projection
// -----------------------------------------------------------------------------------------------------------------------------

float3 BoxProjectReflection(
    float3 worldPosition,
    float3 reflectionDirection,
    float3 probePosition,
    float3 boxMin,
    float3 boxMax)
{
    float3 direction = SafeNormalize(reflectionDirection);

    bool insideBox =
        worldPosition.x >= boxMin.x && worldPosition.x <= boxMax.x &&
        worldPosition.y >= boxMin.y && worldPosition.y <= boxMax.y &&
        worldPosition.z >= boxMin.z && worldPosition.z <= boxMax.z;

    if (!insideBox)
        return direction;

    const float epsilon = 0.00001f;

    float3 safeDirection;

    safeDirection.x = abs(direction.x) > epsilon ? direction.x : (direction.x >= 0.0f ? epsilon : -epsilon);
    safeDirection.y = abs(direction.y) > epsilon ? direction.y : (direction.y >= 0.0f ? epsilon : -epsilon);
    safeDirection.z = abs(direction.z) > epsilon ? direction.z : (direction.z >= 0.0f ? epsilon : -epsilon);

    float3 t0 = (boxMin - worldPosition) / safeDirection;
    float3 t1 = (boxMax - worldPosition) / safeDirection;

    float3 tFar = max(t0, t1);

    float distanceToBox = min(tFar.x, min(tFar.y, tFar.z));

    float3 intersectionPosition = worldPosition + direction * distanceToBox;

    return SafeNormalize(intersectionPosition - probePosition);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Reflection Probe Weight
// -----------------------------------------------------------------------------------------------------------------------------

float GetReflectionProbeInfluence(float3 worldPosition, float3 boxMin, float3 boxMax, float blendDistance)
{
    float3 edgeDistance = min(worldPosition - boxMin, boxMax - worldPosition);
    float distanceToEdge = min(edgeDistance.x, min(edgeDistance.y, edgeDistance.z));

    // Also fade vertically; probes must not influence floors outside their volume.
    return smoothstep(0.0f, max(blendDistance, 0.0001f), distanceToEdge);
}


float3 EvaluateAmbient(
    float3 worldPosition,
    float3 normal,
    float3 viewDirection,
    float3 albedo,
    float roughness,
    float metallic,
    float ambientStrength,
    float reflectionStrength)
{
    roughness = clamp(roughness, 0.045f, 1.0f);
    metallic = saturate(metallic);

    // -------------------------------------------------------------------------------------------------------------------------
    // Fresnel / material
    // -------------------------------------------------------------------------------------------------------------------------

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float NdotV = saturate(dot(normal, viewDirection));

    float3 fresnel = FresnelSchlickRoughness(NdotV, F0, roughness);

    float3 kS = fresnel;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    // -------------------------------------------------------------------------------------------------------------------------
    // Diffuse IBL
    // -------------------------------------------------------------------------------------------------------------------------
    
    const float3 rawIrradiance = irradianceMap.SampleLevel(environmentSampler, normal, 0.0f).rgb;
    
    // Detect directions receiving almost no environment illumination.
    const float irradianceLuminance = dot(rawIrradiance, float3(0.2126f, 0.7152f, 0.0722f));
    
    const float darkSurfaceMask = 1.0f - smoothstep(0.015f, 0.060f, irradianceLuminance);
    
    // Minimum night illumination so surfaces never become completely black.
    const float3 ambientFloor = float3(0.065f, 0.080f, 0.115f);
    
    float3 irradiance = max(rawIrradiance, ambientFloor);
    
    float3 diffuseAmbient = kD * albedo * irradiance;
    
    // Small stylized night fill.
    // Only affects surfaces facing directions with almost no environment illumination.
    const float3 readableAlbedo = lerp(albedo, sqrt(max(albedo, 0.0f)), 0.35f);

    diffuseAmbient += kD * readableAlbedo * float3(0.04f, 0.05f, 0.07f) * darkSurfaceMask;

    // -------------------------------------------------------------------------------------------------------------------------
    // Specular IBL
    // -------------------------------------------------------------------------------------------------------------------------

    float3 reflectionDirection = reflect(-viewDirection, normal);

    // -------------------------------------------------------------------------------------------------------------------------
    // Global environment
    // -------------------------------------------------------------------------------------------------------------------------

    const float maxEnvironmentMip = 7.0f;

    float environmentMipLevel = roughness * maxEnvironmentMip;

    float3 environmentColor = environmentMap.SampleLevel(
        environmentSampler,
        reflectionDirection,
        environmentMipLevel
    ).rgb;

    // -------------------------------------------------------------------------------------------------------------------------
    // Local reflection probes
    // -------------------------------------------------------------------------------------------------------------------------

    float3 localProbeColor = 0.0f;
    float totalProbeWeight = 0.0f;
    float probeCoverage = 0.0f;
    float probeWeightSharpness = lerp(4.0f, 1.0f, smoothstep(0.15f, 0.60f, roughness));

    // Constant descriptor indices avoid requiring descriptor-indexing device features.
    // Evaluate influence per fragment so large meshes do not jump at instance boundaries.
    [unroll]
    for (uint probeIndex = 0; probeIndex < MAX_REFLECTION_PROBES; ++probeIndex)
    {
        if (probeIndex < min(reflectionProbeCount, MAX_REFLECTION_PROBES))
        {
            ReflectionProbeData probe = reflectionProbes[probeIndex];
            float influence = GetReflectionProbeInfluence(
                worldPosition, probe.boxMinBlendDistance.xyz, probe.boxMax.xyz, probe.boxMinBlendDistance.w);

            if (influence > 0.0f)
            {
                float3 halfExtent = max((probe.boxMax.xyz - probe.boxMinBlendDistance.xyz) * 0.5f, 0.0001f);
                float3 normalizedOffset = (worldPosition - probe.positionMaxMip.xyz) / halfExtent;
                float centerWeight = rcp(1.0f + dot(normalizedOffset, normalizedOffset) * 4.0f);
                float weight = pow(centerWeight, probeWeightSharpness) * influence;
                float3 direction = reflectionDirection;

                if ((uint)probe.boxMax.w == REFLECTION_PROBE_PROJECTION_BOX)
                {
                    direction = BoxProjectReflection(
                        worldPosition, reflectionDirection, probe.positionMaxMip.xyz,
                        probe.boxMinBlendDistance.xyz, probe.boxMax.xyz);
                }

                float3 probeColor = reflectionProbeMaps[probeIndex].SampleLevel(
                    environmentSampler, direction, roughness * probe.positionMaxMip.w).rgb;
                localProbeColor += probeColor * weight;
                totalProbeWeight += weight;
                probeCoverage = max(probeCoverage, influence);
            }
        }
    }

    float3 prefilteredColor = environmentColor;

    if (totalProbeWeight > 0.0f)
    {
        // Coverage is independent of center priority and material roughness.
        prefilteredColor = lerp(environmentColor, localProbeColor / totalProbeWeight, probeCoverage);
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Split-sum BRDF
    // -------------------------------------------------------------------------------------------------------------------------

    float2 brdf = brdfLUT.SampleLevel(brdfSampler, float2(NdotV, roughness), 0.0f).rg;

    float3 specularAmbient = prefilteredColor * (F0 * brdf.x + brdf.y);

    return diffuseAmbient * ambientStrength + specularAmbient * reflectionStrength;
}
// -----------------------------------------------------------------------------------------------------------------------------

float InterleavedGradientNoise(float2 position)
{
    return frac(52.9829189f * frac(dot(position, float2(0.06711056f, 0.00583715f))));
}


// -----------------------------------------------------------------------------------------------------------------------------
// Pixel Shader
// -----------------------------------------------------------------------------------------------------------------------------

float SampleAmbientOcclusion(float2 pixelPosition, float viewDepth)
{
    uint width;
    uint height;
    occlusionFiltered.GetDimensions(width, height);
    float2 position = pixelPosition * viewportSize.zw * float2(width, height) - 0.5f;
    int2 base = (int2)floor(position);
    float2 f = frac(position);
    float visibility = 0.0f;
    float totalWeight = 0.0f;

    [unroll]
    for (int y = 0; y < 2; ++y)
    {
        [unroll]
        for (int x = 0; x < 2; ++x)
        {
            int2 coord = clamp(base + int2(x, y), int2(0, 0), int2(width, height) - 1);
            float2 sample = occlusionFiltered.Load(int3(coord, 0)).rg;
            float weight = (x == 0 ? 1.0f - f.x : f.x) * (y == 0 ? 1.0f - f.y : f.y);
            weight *= exp(-abs(sample.y - viewDepth) / (0.15f + viewDepth * 0.005f));
            weight *= sample.y > 0.0f ? 1.0f : 0.0f;
            visibility += sample.x * weight;
            totalWeight += weight;
        }
    }

    return totalWeight > 0.0001f ? visibility / totalWeight : 1.0f;
}

float4 PSNormalDepth(NormalDepthVSOutput input) : SV_Target
{
    if (input.sky != 0)
    {
        discard;
    }

    if (input.terrain == 0 && input.preserveAtDistance == 0)
    {
        float coverage = 1.0f - smoothstep(c_detailFadeStart, c_detailFadeEnd, length(input.worldPosition - cameraPosition.xyz));
        if (coverage <= 0.0f || InterleavedGradientNoise(input.position.xy) > coverage)
        {
            discard;
        }
    }

    float3 normal = SafeNormalize(mul((float3x3)viewMatrix, SafeNormalize(input.worldNormal)));
    float depth = -mul(viewMatrix, float4(input.worldPosition, 1.0f)).z;
    return float4(normal, depth);
}

struct OcclusionOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

OcclusionOutput VSOcclusion(uint vertexID : SV_VertexID)
{
    OcclusionOutput output;
    output.uv = float2(vertexID == 2 ? 2.0f : 0.0f, vertexID == 1 ? 2.0f : 0.0f);
    output.position = float4(output.uv * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

float3 OcclusionViewPosition(float2 uv, float depth)
{
    return float3((uv * 2.0f - 1.0f) * depth / float2(projMatrix[0][0], projMatrix[1][1]), -depth);
}

float4 PSOcclusion(OcclusionOutput input) : SV_Target
{
    uint width;
    uint height;
    occlusionGeometry.GetDimensions(width, height);
    int2 size = int2(width, height);
    int2 coord = min((int2)(input.uv * size), size - 1);
    float4 geometry = occlusionGeometry.Load(int3(coord, 0));
    if (geometry.w <= 0.0f)
    {
        return float4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    float2 centerUV = (float2(coord) + 0.5f) / size;
    float3 position = OcclusionViewPosition(centerUV, geometry.w);
    float radiusPixels = min(72.0f, c_occlusionRadius * abs(projMatrix[1][1]) * height * 0.5f / geometry.w);
    float angle = InterleavedGradientNoise(input.position.xy) * 6.2831853f;
    float occlusion = 0.0f;

    [unroll]
    for (uint i = 0; i < 16; ++i)
    {
        float sampleAngle = angle + float(i % 8) * 0.7853982f;
        float radius = radiusPixels * (i < 8 ? 0.35f : 0.85f);
        int2 sampleCoord = coord + (int2)round(float2(cos(sampleAngle), sin(sampleAngle)) * radius);
        if (any(sampleCoord < 0) || any(sampleCoord >= size))
        {
            continue;
        }

        float depth = occlusionGeometry.Load(int3(sampleCoord, 0)).w;
        if (depth <= 0.0f)
        {
            continue;
        }

        float3 delta = OcclusionViewPosition((float2(sampleCoord) + 0.5f) / size, depth) - position;
        float distanceSquared = dot(delta, delta);
        float horizon = max(dot(geometry.xyz, delta) * rsqrt(max(distanceSquared, 0.0001f)) - 0.08f, 0.0f);
        occlusion += horizon * saturate(1.0f - distanceSquared / (c_occlusionRadius * c_occlusionRadius));
    }

    float visibility = max(c_occlusionMinVisibility, 1.0f - occlusion * c_occlusionStrength / 16.0f);
    return float4(visibility, geometry.w, 0.0f, 0.0f);
}

float4 PSOcclusionBlur(OcclusionOutput input) : SV_Target
{
    uint width;
    uint height;
    occlusionRaw.GetDimensions(width, height);
    int2 size = int2(width, height);
    int2 coord = min((int2)(input.uv * size), size - 1);
    float2 center = occlusionRaw.Load(int3(coord, 0)).rg;
    float visibility = 0.0f;
    float totalWeight = 0.0f;

    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            float2 sample = occlusionRaw.Load(int3(clamp(coord + int2(x, y), int2(0, 0), size - 1), 0)).rg;
            float weight = exp(-float(x * x + y * y) * 0.5f - abs(sample.y - center.y) / (0.15f + center.y * 0.005f));
            visibility += sample.x * weight;
            totalWeight += weight;
        }
    }

    return float4(visibility / max(totalWeight, 0.0001f), center.y, 0.0f, 0.0f);
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (input.sky != 0)
        return float4(EvaluateNightSky(input.worldPosition), 1.0f);

    if (input.terrain == 0 && input.preserveAtDistance == 0)
    {
        const float distanceToCamera = length(input.worldPosition - cameraPosition.xyz);
        const float coverage = 1.0f - smoothstep(c_detailFadeStart, c_detailFadeEnd, distanceToCamera);
        if (coverage <= 0.0f || InterleavedGradientNoise(input.position.xy) > coverage)
            discard;
    }

    float3 normal = SafeNormalize(input.worldNormal);
    float3 viewDirection = SafeNormalize(cameraPosition.xyz - input.worldPosition);

    // -------------------------------------------------------------------------------------------------------------------------
    // Defaults
    // -------------------------------------------------------------------------------------------------------------------------

    float3 albedo = float3(1.0f, 1.0f, 1.0f);

    float roughness = 0.5f;
    float metallic = 0.0f;

    float lightWrap = 0.0f;
    float shapeContrast = 1.0f;

    float ambientStrength = 1.0f;

    float3 emissiveColor = float3(0.0f, 0.0f, 0.0f);
    float emissiveStrength = 0.0f;

    // -------------------------------------------------------------------------------------------------------------------------
    // Material
    // -------------------------------------------------------------------------------------------------------------------------

    if (input.materialIndex >= 0 && input.materialIndex < int(materialCount))
    {
        MaterialData material = materials[input.materialIndex];

        albedo = material.albedo.rgb;
        ambientStrength = max(material.albedo.a, 0.0f);

        roughness = clamp(material.properties.x, 0.045f, 1.0f);
        metallic = saturate(material.properties.y);

        lightWrap = saturate(material.properties.z);
        shapeContrast = max(material.properties.w, 0.05f);

        emissiveColor = material.emissiveColor.rgb;
        emissiveStrength = max(material.emissiveColor.a, 0.0f);
    }

    // Optional: Shape-/Instance-Farbe als Material-Tint verwenden.

    albedo *= input.color.rgb;

    ApplyForestSurface(input.worldPosition, normal, input.surfaceFlags, metallic, emissiveStrength, albedo, roughness);

    const float crystalMask = input.crystal != 0 ? 1.0f : 0.0f;

    if (crystalMask > 0.0f)
    {
        roughness = min(roughness, 0.16f);
        metallic = max(metallic, 0.72f);
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Ambient
    // -------------------------------------------------------------------------------------------------------------------------

    float3 finalColor = EvaluateAmbient(
        input.worldPosition,
        normal,
        viewDirection,
        albedo,
        roughness,
        metallic,
        ambientStrength * c_ambientLightStrength,
        lerp(1.0f, 2.5f, crystalMask)
    );
    finalColor *= SampleAmbientOcclusion(input.position.xy, GetCameraViewDepth(input.worldPosition));
    // -------------------------------------------------------------------------------------------------------------------------
    // Direct Lighting
    // -------------------------------------------------------------------------------------------------------------------------

    for (uint activeIndex = 0; activeIndex < activeLightCount; ++activeIndex)
    {
        uint index = activeLightIndices[activeIndex];
        LightData light = lights[index];

        uint lightType = (uint) light.directionType.w;

        if (lightType == LIGHT_TYPE_DIRECTIONAL)
        {
            float3 contribution = EvaluateDirectionalLight(input.worldPosition, normal, albedo, light, roughness, metallic, lightWrap, shapeContrast);

            if (light.shadowIndex >= 0)
            {
                uint shadowIndex = (uint) light.shadowIndex;

                ShadowData shadowData = shadows[shadowIndex];

                float viewDepth = GetCameraViewDepth(input.worldPosition);
                uint cascadeIndex = GetDirectionalCascadeIndex(viewDepth, shadowData);

                if (cascadeIndex < shadowData.matrixCount)
                {
                    float3 lightDirection = SafeNormalize(-light.directionType.xyz);

                    float shadow = CalculateShadow(input.worldPosition, normal, lightDirection, shadowIndex, cascadeIndex);

                    float cascadeBlend = GetCascadeBlendFactor(viewDepth, cascadeIndex, shadowData);

                    if (cascadeBlend > 0.0f && cascadeIndex + 1 < shadowData.matrixCount)
                    {
                        float nextShadow = CalculateShadow(input.worldPosition, normal, lightDirection, shadowIndex, cascadeIndex + 1);

                        shadow = lerp(shadow, nextShadow, cascadeBlend);
                    }

                    const float shadowStrength = 0.75f;

                    contribution *= 1.0f - shadow * shadowStrength;
                }
            }

            finalColor += contribution;
        }
        else if (lightType == LIGHT_TYPE_POINT)
        {
            float3 contribution = EvaluatePointLight(input.worldPosition, normal, albedo, light, roughness, metallic, lightWrap, shapeContrast);

            if (light.shadowIndex >= 0)
            {
                float3 lightDirection = SafeNormalize(light.positionRadius.xyz - input.worldPosition);

                uint matrixIndex = GetPointShadowMatrixIndex(input.worldPosition, light.positionRadius.xyz);

                float shadow = CalculateShadow(input.worldPosition, normal, lightDirection, (uint) light.shadowIndex, matrixIndex);

                contribution *= 1.0f - shadow;
            }

            finalColor += contribution;
        }
        else if (lightType == LIGHT_TYPE_SPOT)
        {
            float3 contribution = EvaluateSpotLight(input.worldPosition, normal, albedo, light, roughness, metallic, lightWrap, shapeContrast);

            if (light.shadowIndex >= 0)
            {
                float3 lightDirection = SafeNormalize(light.positionRadius.xyz - input.worldPosition);

                float shadow = CalculateShadow(input.worldPosition, normal, lightDirection, (uint) light.shadowIndex, 0);

                contribution *= 1.0f - shadow;
            }

            finalColor += contribution;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Emissive
    // -------------------------------------------------------------------------------------------------------------------------

    const float crystalFacet = 0.72f + 0.28f * abs(dot(normal, float3(0.577f, 0.577f, 0.577f)));

    finalColor += emissiveColor * emissiveStrength * (1.0f + crystalMask * crystalFacet * 0.12f);

    // The same display-linear color clears the background, hiding the outer terrain edge.
    const float fogDistance = length(input.worldPosition - cameraPosition.xyz);
    const float fogDepth = max(fogDistance - c_fogStart, 0.0f);
    const float cameraHeight = max(cameraPosition.y - c_heightFogBaseHeight, 0.0f);
    const float fragmentHeight = max(input.worldPosition.y - c_heightFogBaseHeight, 0.0f);
    const float averageHeight = 0.5f * (cameraHeight + fragmentHeight);
    const float heightFogFactor = exp(-averageHeight * c_heightFogFalloff);
    const float fogExtinction = c_fogDensity + c_heightFogDensity * heightFogFactor;
    const float fogAmount = input.preserveAtDistance != 0 ? 0.0f :
        max(1.0f - exp(-fogDepth * fogExtinction), smoothstep(c_fogEdgeStart, c_fogEnd, fogDistance));
    const float3 fogColor = float3(c_fogRed, c_fogGreen, c_fogBlue);
    finalColor = lerp(finalColor, fogColor, fogAmount);

    return float4(clamp(finalColor, 0.0f, 65504.0f), input.color.a);
}
