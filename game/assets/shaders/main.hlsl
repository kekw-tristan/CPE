static const uint MAX_REFLECTION_PROBES = 8;

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
    uint padding1;

    ReflectionProbeData reflectionProbes[MAX_REFLECTION_PROBES];
};


struct InstanceData
{
    row_major float4x4 worldMatrix;

    float4 color;

    int materialIndex;
    int padding1;
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

    nointerpolation uint2  reflectionProbeIndices  : TEXCOORD1;
    nointerpolation float2 reflectionProbeWeights  : TEXCOORD2;
    nointerpolation float  reflectionProbeCoverage : TEXCOORD3;

    nointerpolation int materialIndex : MATERIAL_INDEX;
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


float GetReflectionProbeWeight(float3 worldPosition, float3 probePosition, float3 boxMin, float3 boxMax, float blendDistance);

// -----------------------------------------------------------------------------------------------------------------------------
// Vertex Shader
// -----------------------------------------------------------------------------------------------------------------------------

VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output;

    InstanceData instance = instances[instanceID];

    float4 worldPosition = mul(float4(input.position, 1.0f), instance.worldMatrix);

    output.position = mul(viewProj, worldPosition);
    output.worldPosition = worldPosition.xyz;

    float3x3 normalMatrix = (float3x3) instance.worldMatrix;

    output.worldNormal = SafeNormalize(mul(input.normal, normalMatrix));

    output.texCoord = input.texCoord;
    output.color = instance.color;

    // -------------------------------------------------------------------------------------------------------------------------
    // Reflection probe selection
    //
    // The local origin transformed by the instance matrix is used as one stable anchor for the complete instance.
    // -------------------------------------------------------------------------------------------------------------------------

    float3 reflectionProbeAnchor = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), instance.worldMatrix).xyz;

    uint bestProbeIndex0 = 0;
    uint bestProbeIndex1 = 0;

    float bestProbeWeight0 = 0.0f;
    float bestProbeWeight1 = 0.0f;

    uint activeProbeCount = min(reflectionProbeCount, MAX_REFLECTION_PROBES);

    for (uint probeIndex = 0; probeIndex < activeProbeCount; ++probeIndex)
    {
        ReflectionProbeData probe = reflectionProbes[probeIndex];

        float probeWeight = GetReflectionProbeWeight(
            reflectionProbeAnchor,
            probe.positionMaxMip.xyz,
            probe.boxMinBlendDistance.xyz,
            probe.boxMax.xyz,
            probe.boxMinBlendDistance.w
        );

        if (probeWeight > bestProbeWeight0)
        {
            bestProbeWeight1 = bestProbeWeight0;
            bestProbeIndex1 = bestProbeIndex0;

            bestProbeWeight0 = probeWeight;
            bestProbeIndex0 = probeIndex;
        }
        else if (probeWeight > bestProbeWeight1)
        {
            bestProbeWeight1 = probeWeight;
            bestProbeIndex1 = probeIndex;
        }
    }

    output.reflectionProbeIndices = uint2(bestProbeIndex0, bestProbeIndex1);
    output.reflectionProbeWeights = float2(bestProbeWeight0, bestProbeWeight1);
    output.reflectionProbeCoverage = saturate(bestProbeWeight0);

    output.materialIndex = instance.materialIndex;

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

float GetReflectionProbeWeight(float3 worldPosition, float3 probePosition, float3 boxMin, float3 boxMax, float blendDistance)
{
    bool insideBox =
        worldPosition.x >= boxMin.x && worldPosition.x <= boxMax.x &&
        worldPosition.z >= boxMin.z && worldPosition.z <= boxMax.z;

    if (!insideBox)
        return 0.0f;

    // -------------------------------------------------------------------------------------------------------------------------
    // Center distance
    //
    // Probes close to their capture position get a higher priority than probes whose influence box only happens to overlap.
    // -------------------------------------------------------------------------------------------------------------------------

    float2 halfExtent = max((boxMax.xz - boxMin.xz) * 0.5f, float2(0.0001f, 0.0001f));

    float2 normalizedOffset = abs(worldPosition.xz - probePosition.xz) / halfExtent;

    float normalizedDistanceSquared = dot(normalizedOffset, normalizedOffset);

    float centerWeight = 1.0f / (1.0f + normalizedDistanceSquared * 4.0f);

    // -------------------------------------------------------------------------------------------------------------------------
    // Influence box edge fade
    //
    // Center weighting alone must not be combined with a hard box cutoff, otherwise a visible seam appears at the box edge.
    // -------------------------------------------------------------------------------------------------------------------------

    if (blendDistance <= 0.0001f)
        return centerWeight;

    float distanceX = min(worldPosition.x - boxMin.x, boxMax.x - worldPosition.x);
    float distanceZ = min(worldPosition.z - boxMin.z, boxMax.z - worldPosition.z);

    float edgeWeightX = smoothstep(0.0f, blendDistance, distanceX);
    float edgeWeightZ = smoothstep(0.0f, blendDistance, distanceZ);

    float edgeWeight = edgeWeightX * edgeWeightZ;

    return centerWeight * edgeWeight;
}


float3 EvaluateAmbient(
    float3 worldPosition,
    uint2 reflectionProbeIndices,
    float2 reflectionProbeWeights,
    float reflectionProbeCoverage,
    float3 normal,
    float3 viewDirection,
    float3 albedo,
    float roughness,
    float metallic,
    float ambientStrength)
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

    float3 irradiance = irradianceMap.SampleLevel(environmentSampler, normal, 0.0f).rgb;

    float3 diffuseAmbient = kD * albedo * irradiance;

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

    float3 localProbeColor = float3(0.0f, 0.0f, 0.0f);
    float totalProbeWeight = 0.0f;

    // Smooth surfaces should strongly favor the dominant probe.
    // On mirror-like surfaces the secondary probe is disabled completely to avoid parallax-correction ghosting.

    float roughnessBlend = smoothstep(0.15f, 0.60f, roughness);
    float probeWeightSharpness = lerp(8.0f, 1.0f, roughnessBlend);

    float probeWeight0 = pow(reflectionProbeWeights.x, probeWeightSharpness);
    float probeWeight1 = pow(reflectionProbeWeights.y, probeWeightSharpness) * roughnessBlend;

    // -------------------------------------------------------------------------------------------------------------------------
    // Primary probe
    // -------------------------------------------------------------------------------------------------------------------------

    if (probeWeight0 > 0.0001f)
    {
        uint probeIndex = reflectionProbeIndices.x;

        ReflectionProbeData probe = reflectionProbes[probeIndex];

        uint projectionType = (uint) probe.boxMax.w;

        float3 correctedReflectionDirection = reflectionDirection;

        if (projectionType == REFLECTION_PROBE_PROJECTION_BOX)
        {
            correctedReflectionDirection = BoxProjectReflection(
                worldPosition,
                reflectionDirection,
                probe.positionMaxMip.xyz,
                probe.boxMinBlendDistance.xyz,
                probe.boxMax.xyz
            );
        }

        const float minimumProbeRoughness = 0.20f;

        float probeRoughness = max(roughness, minimumProbeRoughness);
        float mipLevel = probeRoughness * probe.positionMaxMip.w;

        float3 probeColor = reflectionProbeMaps[probeIndex].SampleLevel(
            environmentSampler,
            correctedReflectionDirection,
            mipLevel
        ).rgb;

        localProbeColor += probeColor * probeWeight0;
        totalProbeWeight += probeWeight0;
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Secondary probe
    // -------------------------------------------------------------------------------------------------------------------------

    if (probeWeight1 > 0.0001f)
    {
        uint probeIndex = reflectionProbeIndices.y;

        ReflectionProbeData probe = reflectionProbes[probeIndex];
        
        uint projectionType = (uint) probe.boxMax.w;

        float3 correctedReflectionDirection = reflectionDirection;

        if (projectionType == REFLECTION_PROBE_PROJECTION_BOX)
        {
                    correctedReflectionDirection = BoxProjectReflection(
                worldPosition,
                reflectionDirection,
                probe.positionMaxMip.xyz,
                probe.boxMinBlendDistance.xyz,
                probe.boxMax.xyz
            );
        }

        const float minimumProbeRoughness = 0.20f;

        float probeRoughness = max(roughness, minimumProbeRoughness);
        float mipLevel = probeRoughness * probe.positionMaxMip.w;

        float3 probeColor = reflectionProbeMaps[probeIndex].SampleLevel(
            environmentSampler,
            correctedReflectionDirection,
            mipLevel
        ).rgb;

        localProbeColor += probeColor * probeWeight1;
        totalProbeWeight += probeWeight1;
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Local / global blend
    // -------------------------------------------------------------------------------------------------------------------------

    float3 prefilteredColor = environmentColor;

    if (totalProbeWeight > 0.0001f)
    {
        localProbeColor /= totalProbeWeight;

        prefilteredColor = lerp(environmentColor, localProbeColor, saturate(reflectionProbeCoverage));
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Split-sum BRDF
    // -------------------------------------------------------------------------------------------------------------------------

    float2 brdf = brdfLUT.SampleLevel(brdfSampler, float2(NdotV, roughness), 0.0f).rg;

    float3 specularAmbient = prefilteredColor * (F0 * brdf.x + brdf.y);

    return (diffuseAmbient + specularAmbient) * ambientStrength;
}
// -----------------------------------------------------------------------------------------------------------------------------

float InterleavedGradientNoise(float2 position)
{
    return frac(52.9829189f * frac(dot(position, float2(0.06711056f, 0.00583715f))));
}


// -----------------------------------------------------------------------------------------------------------------------------
// Tone Mapping
// -----------------------------------------------------------------------------------------------------------------------------

float3 ACESFilm(float3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}


// -----------------------------------------------------------------------------------------------------------------------------
// Pixel Shader
// -----------------------------------------------------------------------------------------------------------------------------

float4 PSMain(VSOutput input) : SV_Target
{
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

    // -------------------------------------------------------------------------------------------------------------------------
    // Ambient
    // -------------------------------------------------------------------------------------------------------------------------

    float3 finalColor = EvaluateAmbient(
        input.worldPosition,
        input.reflectionProbeIndices,
        input.reflectionProbeWeights,
        input.reflectionProbeCoverage,
        normal,
        viewDirection,
        albedo,
        roughness,
        metallic,
        ambientStrength
    );
    // -------------------------------------------------------------------------------------------------------------------------
    // Direct Lighting
    // -------------------------------------------------------------------------------------------------------------------------

    for (uint index = 0; index < lightCount; ++index)
    {
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

                    float shadowStrength = 0.75f;

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

    finalColor += emissiveColor * emissiveStrength;

    // -------------------------------------------------------------------------------------------------------------------------
    // Tone Mapping
    // -------------------------------------------------------------------------------------------------------------------------

    finalColor = ACESFilm(finalColor);

    float dither = InterleavedGradientNoise(input.position.xy) - 0.5f;

    finalColor += dither / 255.0f;

    return float4(saturate(finalColor), input.color.a);
}