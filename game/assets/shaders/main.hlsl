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
};


[[vk::binding(2, 0)]]
StructuredBuffer<LightData> lights;


struct MaterialData
{
    float4 albedo;
    float4 properties;
    float4 emissiveColor;
};


[[vk::binding(3, 0)]]
StructuredBuffer<MaterialData> materials;


static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;


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
    uint materialIndex : MATERIAL_INDEX;
};


VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output;

    InstanceData instance = instances[instanceID];

    float4 worldPosition = mul(float4(input.position, 1.0f), instance.worldMatrix);

    output.position = mul(viewProj, worldPosition);
    output.worldPosition = worldPosition.xyz;

    float3x3 normalMatrix = (float3x3)instance.worldMatrix;
    output.worldNormal = normalize(mul(input.normal, normalMatrix));

    output.texCoord = input.texCoord;

    // Bleibt weiterhin im InstanceData, wird aber nicht mehr
    // zur Bestimmung der Materialfarbe verwendet.
    output.color = instance.color;

    output.materialIndex = instance.materialIndex;

    return output;
}


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
    float3 lightDirection = normalize(-light.directionType.xyz);

    float NdotL = saturate(dot(normal, lightDirection));

    float wrappedNdotL = saturate((NdotL + lightWrap) / (1.0f + lightWrap));

    float shapedNdotL = lerp(NdotL, wrappedNdotL, shapeContrast);

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    float3 diffuse = albedo * lightColor * lightIntensity * shapedNdotL;

    float3 viewDirection = normalize(cameraPosition.xyz - worldPosition);
    float3 halfVector = normalize(lightDirection + viewDirection);

    float NdotH = saturate(dot(normal, halfVector));

    float specularPower = lerp(4.0f, 128.0f, 1.0f - roughness);
    float specular = pow(NdotH, specularPower);

    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 specularContribution = specularColor * specular * lightColor * lightIntensity * NdotL;

    return diffuse + specularContribution;
}


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

    float distanceToLight = length(toLight);
    float radius = light.positionRadius.w;

    if (distanceToLight >= radius)
        return float3(0.0f, 0.0f, 0.0f);

    float3 lightDirection = toLight / max(distanceToLight, 0.0001f);

    float NdotL = saturate(dot(normal, lightDirection));

    float wrappedNdotL = saturate((NdotL + lightWrap) / (1.0f + lightWrap));

    float shapedNdotL = lerp(NdotL, wrappedNdotL, shapeContrast);

    float attenuation = saturate(1.0f - distanceToLight / radius);
    attenuation *= attenuation;

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    float3 diffuse = albedo * lightColor * lightIntensity * shapedNdotL * attenuation;

    float3 viewDirection = normalize(cameraPosition.xyz - worldPosition);
    float3 halfVector = normalize(lightDirection + viewDirection);

    float NdotH = saturate(dot(normal, halfVector));

    float specularPower = lerp(4.0f, 128.0f, 1.0f - roughness);
    float specular = pow(NdotH, specularPower);

    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 specularContribution = specularColor * specular * lightColor * lightIntensity * NdotL * attenuation;

    return diffuse + specularContribution;
}


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

    float distanceToLight = length(toLight);
    float radius = light.positionRadius.w;

    if (distanceToLight >= radius)
        return float3(0.0f, 0.0f, 0.0f);

    float3 lightDirection = toLight / max(distanceToLight, 0.0001f);

    float NdotL = saturate(dot(normal, lightDirection));

    float wrappedNdotL = saturate((NdotL + lightWrap) / (1.0f + lightWrap));

    float shapedNdotL = lerp(NdotL, wrappedNdotL, shapeContrast);

    float attenuation = saturate(1.0f - distanceToLight / radius);
    attenuation *= attenuation;

    float3 spotDirection = normalize(light.directionType.xyz);

    float cosAngle = dot(-lightDirection, spotDirection);

    float innerCone = light.spotData.x;
    float outerCone = light.spotData.y;

    float spotFactor = saturate((cosAngle - outerCone) / max(innerCone - outerCone, 0.0001f));

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    float3 diffuse = albedo * lightColor * lightIntensity * shapedNdotL * attenuation * spotFactor;

    float3 viewDirection = normalize(cameraPosition.xyz - worldPosition);
    float3 halfVector = normalize(lightDirection + viewDirection);

    float NdotH = saturate(dot(normal, halfVector));

    float specularPower = lerp(4.0f, 128.0f, 1.0f - roughness);
    float specular = pow(NdotH, specularPower);

    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 specularContribution = specularColor * specular * lightColor * lightIntensity * NdotL * attenuation * spotFactor;

    return diffuse + specularContribution;
}


float4 PSMain(VSOutput input) : SV_Target
{
    float3 normal = normalize(input.worldNormal);

    // Material-Defaultwerte
    float3 albedo = float3(1.0f, 1.0f, 1.0f);

    float roughness = 0.5f;
    float metallic = 0.0f;
    float lightWrap = 0.0f;
    float shapeContrast = 1.0f;
    float ambientStrength = 1.0f;

    float3 emissiveColor = float3(0.0f, 0.0f, 0.0f);
    float emissiveStrength = 0.0f;

    // Material anhand des Instance-Material-Index laden.
    if (input.materialIndex >= 0 && input.materialIndex < materialCount)
    {
        MaterialData material = materials[input.materialIndex];

        albedo = material.albedo.rgb;

        roughness = saturate(material.properties.x);
        metallic = saturate(material.properties.y);
        lightWrap = saturate(material.properties.z);
        shapeContrast = max(material.properties.w, 0.0f);

        emissiveColor = material.emissiveColor.rgb;
        emissiveStrength = material.emissiveColor.w;
    }

    float3 ambientColor = float3(0.08f, 0.10f, 0.15f);

    float3 finalColor = albedo * ambientColor * ambientStrength;

    for (uint index = 0; index < lightCount; ++index)
    {
        LightData light = lights[index];

        uint lightType = (uint)light.directionType.w;

        if (lightType == LIGHT_TYPE_DIRECTIONAL)
        {
            finalColor += EvaluateDirectionalLight(
                input.worldPosition,
                normal,
                albedo,
                light,
                roughness,
                metallic,
                lightWrap,
                shapeContrast);
        }
        else if (lightType == LIGHT_TYPE_POINT)
        {
            finalColor += EvaluatePointLight(
                input.worldPosition,
                normal,
                albedo,
                light,
                roughness,
                metallic,
                lightWrap,
                shapeContrast);
        }
        else if (lightType == LIGHT_TYPE_SPOT)
        {
            finalColor += EvaluateSpotLight(
                input.worldPosition,
                normal,
                albedo,
                light,
                roughness,
                metallic,
                lightWrap,
                shapeContrast);
        }
    }

    finalColor += emissiveColor * emissiveStrength;

    return float4(finalColor, input.color.a);
}