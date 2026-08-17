// -----------------------------------------------------------------------------------------------------------------------------
// Frame data
// -----------------------------------------------------------------------------------------------------------------------------

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
    uint padding0;
    uint padding1;

    float4 reflectionProbePosition;
    float4 reflectionProbeBoxMin;
    float4 reflectionProbeBoxMax;
};


// -----------------------------------------------------------------------------------------------------------------------------
// Reflection probe push constants
// -----------------------------------------------------------------------------------------------------------------------------

struct ReflectionProbePushConstants
{
    row_major float4x4 viewProjection;
    float4 cameraPosition;
};

[[vk::push_constant]]
ReflectionProbePushConstants probePushConstants;



// -----------------------------------------------------------------------------------------------------------------------------
// Instance data
// -----------------------------------------------------------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------------------------------------------------------
// Light data
// -----------------------------------------------------------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------------------------------------------------------
// Material data
// -----------------------------------------------------------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------------------------------------------------------
// Shadow data
// -----------------------------------------------------------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------------------------------------------------------
// IBL
// -----------------------------------------------------------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------------------------------------------------------

static const float PI = 3.14159265359f;

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;


// -----------------------------------------------------------------------------------------------------------------------------
// Vertex data
// -----------------------------------------------------------------------------------------------------------------------------

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};


struct VSOutput
{
    float4 position : SV_Position;

    float3 worldPosition : POSITION0;
    float3 worldNormal : NORMAL0;

    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;

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


// -----------------------------------------------------------------------------------------------------------------------------
// Vertex shader
// -----------------------------------------------------------------------------------------------------------------------------

VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output;

    InstanceData instance = instances[instanceID];

    float4 worldPosition = mul(float4(input.position, 1.0f), instance.worldMatrix);

    output.position = mul(worldPosition, probePushConstants.viewProjection);
    output.worldPosition = worldPosition.xyz;

    float3x3 normalMatrix = (float3x3) instance.worldMatrix;

    output.worldNormal = SafeNormalize(mul(input.normal, normalMatrix));
    output.texCoord = input.texCoord;
    output.color = instance.color;
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

    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * factor;
}


// -----------------------------------------------------------------------------------------------------------------------------
// Shadows
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

    uint width;
    uint height;
    uint layers;

    shadowMap.GetDimensions(width, height, layers);

    float2 texelSize = 1.0f / float2(width, height);

    float shadow = 0.0f;

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
        return direction.x >= 0.0f ? 0 : 1;

    if (absDirection.y >= absDirection.x && absDirection.y >= absDirection.z)
        return direction.y >= 0.0f ? 2 : 3;

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

float GetMainCameraViewDepth(float3 worldPosition)
{
    return dot(worldPosition - cameraPosition.xyz, SafeNormalize(cameraDirection.xyz));
}


// -----------------------------------------------------------------------------------------------------------------------------
// Shape lighting
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
    float3 viewDirection,
    float3 lightDirection,
    float3 radiance,
    float3 albedo,
    float roughness,
    float metallic,
    float lightWrap,
    float shapeContrast)
{
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
// Directional light
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateDirectionalLight(
    float3 worldPosition,
    float3 normal,
    float3 viewDirection,
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

    return EvaluateSurfaceLight(worldPosition, normal, viewDirection, lightDirection, radiance, albedo, roughness, metallic, lightWrap, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Point light
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluatePointLight(
    float3 worldPosition,
    float3 normal,
    float3 viewDirection,
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

    return EvaluateSurfaceLight(worldPosition, normal, viewDirection, lightDirection, radiance, albedo, roughness, metallic, lightWrap, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Spot light
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateSpotLight(
    float3 worldPosition,
    float3 normal,
    float3 viewDirection,
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

    return EvaluateSurfaceLight(worldPosition, normal, viewDirection, lightDirection, radiance, albedo, roughness, metallic, lightWrap, shapeContrast);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Ambient / global IBL
// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateAmbient(float3 normal, float3 viewDirection, float3 albedo, float roughness, float metallic, float ambientStrength)
{
    roughness = clamp(roughness, 0.045f, 1.0f);
    metallic = saturate(metallic);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float NdotV = saturate(dot(normal, viewDirection));

    float3 fresnel = FresnelSchlickRoughness(NdotV, F0, roughness);

    float3 kS = fresnel;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    // Diffuse IBL
    float3 irradiance = irradianceMap.SampleLevel(environmentSampler, normal, 0.0f).rgb;
    float3 diffuseAmbient = kD * albedo * irradiance;

    // Specular IBL
    float3 reflectionDirection = reflect(-viewDirection, normal);

    const float maxMipLevel = 7.0f;
    float mipLevel = roughness * maxMipLevel;

    float3 prefilteredColor = environmentMap.SampleLevel(environmentSampler, reflectionDirection, mipLevel).rgb;

    float2 brdf = brdfLUT.SampleLevel(brdfSampler, float2(NdotV, roughness), 0.0f).rg;

    float3 specularAmbient = prefilteredColor * (F0 * brdf.x + brdf.y);

    return (diffuseAmbient + specularAmbient) * ambientStrength;
}


// -----------------------------------------------------------------------------------------------------------------------------
// Pixel shader
// -----------------------------------------------------------------------------------------------------------------------------

float4 PSMain(VSOutput input) : SV_Target
{
    float3 normal = SafeNormalize(input.worldNormal);
    float3 viewDirection = SafeNormalize(probePushConstants.cameraPosition.xyz - input.worldPosition);

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

    albedo *= input.color.rgb;

    // -------------------------------------------------------------------------------------------------------------------------
    // Global IBL
    // -------------------------------------------------------------------------------------------------------------------------

    float3 finalColor = EvaluateAmbient(normal, viewDirection, albedo, roughness, metallic, ambientStrength);

    // -------------------------------------------------------------------------------------------------------------------------
    // Direct lighting
    // -------------------------------------------------------------------------------------------------------------------------

    for (uint index = 0; index < lightCount; ++index)
    {
        LightData light = lights[index];

        uint lightType = (uint) light.directionType.w;

        if (lightType == LIGHT_TYPE_DIRECTIONAL)
        {
            float3 contribution = EvaluateDirectionalLight(input.worldPosition, normal, viewDirection, albedo, light, roughness, metallic, lightWrap, shapeContrast);

            if (light.shadowIndex >= 0)
            {
                uint shadowIndex = (uint) light.shadowIndex;

                ShadowData shadowData = shadows[shadowIndex];

                // Die Directional-Shadow-Map ist weiterhin an die Main-Camera-CSMs gekoppelt.
                float viewDepth = GetMainCameraViewDepth(input.worldPosition);
                uint cascadeIndex = GetDirectionalCascadeIndex(viewDepth, shadowData);

                if (cascadeIndex < shadowData.matrixCount)
                {
                    float3 lightDirection = SafeNormalize(-light.directionType.xyz);
                    float shadow = CalculateShadow(input.worldPosition, normal, lightDirection, shadowIndex, cascadeIndex);

                    contribution *= 1.0f - shadow;
                }
            }

            finalColor += contribution;
        }
        else if (lightType == LIGHT_TYPE_POINT)
        {
            float3 contribution = EvaluatePointLight(input.worldPosition, normal, viewDirection, albedo, light, roughness, metallic, lightWrap, shapeContrast);

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
            float3 contribution = EvaluateSpotLight(input.worldPosition, normal, viewDirection, albedo, light, roughness, metallic, lightWrap, shapeContrast);

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

    return float4(max(finalColor, 0.0f), 1.0f);
}