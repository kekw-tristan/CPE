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
};


VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output;

    InstanceData instance = instances[instanceID];

    float4 worldPosition = mul(float4(input.position, 1.0f), instance.worldMatrix);

    output.position = mul(viewProj, worldPosition);
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    output.texCoord = input.texCoord;
    output.color = instance.color;

    return output;
}


float3 EvaluateDirectionalLight(float3 normal, float3 albedo, LightData light)
{
    float3 lightDirection = normalize(-light.directionType.xyz);
    float NdotL = saturate(dot(normal, lightDirection));

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    return albedo * lightColor * lightIntensity * NdotL;
}


float3 EvaluatePointLight(float3 worldPosition, float3 normal, float3 albedo, LightData light)
{
    float3 toLight = light.positionRadius.xyz - worldPosition;

    float distanceToLight = length(toLight);
    float radius = light.positionRadius.w;

    if (distanceToLight >= radius)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 lightDirection = toLight / max(distanceToLight, 0.0001f);
    float NdotL = saturate(dot(normal, lightDirection));

    float attenuation = saturate(1.0f - distanceToLight / radius);
    attenuation *= attenuation;

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    return albedo * lightColor * lightIntensity * NdotL * attenuation;
}


float3 EvaluateSpotLight(float3 worldPosition, float3 normal, float3 albedo, LightData light)
{
    float3 toLight = light.positionRadius.xyz - worldPosition;

    float distanceToLight = length(toLight);
    float radius = light.positionRadius.w;

    if (distanceToLight >= radius)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 lightDirection = toLight / max(distanceToLight, 0.0001f);
    float NdotL = saturate(dot(normal, lightDirection));

    float attenuation = saturate(1.0f - distanceToLight / radius);
    attenuation *= attenuation;

    float3 spotDirection = normalize(light.directionType.xyz);
    float cosAngle = dot(-lightDirection, spotDirection);

    float innerCone = light.spotData.x;
    float outerCone = light.spotData.y;

    float spotFactor = saturate((cosAngle - outerCone) / max(innerCone - outerCone, 0.0001f));

    float3 lightColor = light.colorIntensity.rgb;
    float lightIntensity = light.colorIntensity.w;

    return albedo * lightColor * lightIntensity * NdotL * attenuation * spotFactor;
}


float4 PSMain(VSOutput input) : SV_Target
{
    float3 normal = normalize(input.worldNormal);
    float3 albedo = input.color.rgb;

    // temp ambient light
    float3 ambientColor = float3(0.08f, 0.10f, 0.15f);
    float3 finalColor = albedo * ambientColor;

    for (uint index = 0; index < lightCount; ++index)
    {
        LightData light = lights[index];
        uint lightType = (uint) light.directionType.w;

        if (lightType == LIGHT_TYPE_DIRECTIONAL)
        {
            finalColor += EvaluateDirectionalLight(normal, albedo, light);
        }
        else if (lightType == LIGHT_TYPE_POINT)
        {
            finalColor += EvaluatePointLight(input.worldPosition, normal, albedo, light);
        }
        else if (lightType == LIGHT_TYPE_SPOT)
        {
            finalColor += EvaluateSpotLight(input.worldPosition, normal, albedo, light);
        }
    }

    return float4(finalColor, input.color.a);
}