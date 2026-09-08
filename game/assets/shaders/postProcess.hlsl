#include "../../src/world/forestAtmosphere.h"

[[vk::binding(0, 0)]]
Texture2D<float4> sceneColor;

[[vk::binding(1, 0)]]
SamplerState sceneSampler;

[[vk::binding(2, 0)]]
Texture2D<float4> bloomColor;

[[vk::binding(3, 0)]]
SamplerState bloomSampler;

struct PostProcessPushConstants
{
    uint mode; // 0: prefilter, 1: downsample, 2: upsample
};

[[vk::push_constant]]
PostProcessPushConstants pushConstants;

[[vk::constant_id(0)]]
const bool c_encodeSRGB = false;

static const float c_bloomThreshold = 1.0f;
static const float c_bloomSoftKnee = 0.5f;
static const float c_bloomIntensity = 0.35f;
static const float c_bloomScatter = 0.7f;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    const float2 positions[3] =
    {
        float2(-1.0f, -1.0f),
        float2(-1.0f, 3.0f),
        float2(3.0f, -1.0f)
    };

    const float2 uvs[3] =
    {
        float2(0.0f, 0.0f),
        float2(0.0f, 2.0f),
        float2(2.0f, 0.0f)
    };

    VSOutput output;
    output.position = float4(positions[vertexID], 0.0f, 1.0f);
    output.uv = uvs[vertexID];
    return output;
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ACESFilm(float3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 ApplyDisplayGrade(float3 color)
{
    // Keep the rational curve finite even for highlights near the FP16 limit.
    color = ACESFilm(clamp(color * c_sceneExposure, 0.0f, 65504.0f));

    const float luminance = Luminance(color);
    color = lerp(luminance.xxx, color, c_colorSaturation);
    color = (color - 0.5f) * c_colorContrast + 0.5f;
    // Artistic midtone adjustment, separate from the output transfer function.
    color = pow(saturate(color), 1.0f / max(c_displayGamma, 0.0001f));

    return saturate(color);
}

float3 ExtractBloom(float3 color)
{
    color = clamp(color, 0.0f, 65504.0f);
    const float threshold = c_bloomThreshold;
    const float softKnee = max(threshold * c_bloomSoftKnee, 0.0001f);
    // Threshold in exposed space, but keep the bloom buffer in scene-linear HDR.
    const float brightness = max(color.r, max(color.g, color.b)) * c_sceneExposure;
    float softContribution = clamp(brightness - threshold + softKnee, 0.0f, 2.0f * softKnee);
    softContribution = softContribution * softContribution / max(4.0f * softKnee, 0.0001f);

    const float bloomContribution = max(brightness - threshold, softContribution);
    return color * saturate(bloomContribution / max(brightness, 0.0001f));
}

float3 SampleBloomSource(float2 uv)
{
    float3 color = max(sceneColor.SampleLevel(sceneSampler, uv, 0.0f).rgb, 0.0f);
    return pushConstants.mode == 0 ? ExtractBloom(color) : color;
}

float3 UpsampleBloom(float2 uv)
{
    uint width;
    uint height;
    bloomColor.GetDimensions(width, height);
    const float2 texelSize = 1.0f / float2(width, height);
    float3 color = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            const float weight = (x == 0 ? 2.0f : 1.0f) * (y == 0 ? 2.0f : 1.0f) / 16.0f;
            color += bloomColor.SampleLevel(bloomSampler, uv + float2(x, y) * texelSize, 0.0f).rgb * weight;
        }
    }

    return color;
}

float4 PSBloom(VSOutput input) : SV_Target
{
    if (pushConstants.mode == 2)
    {
        // Normalized reconstruction keeps brightness independent of pyramid depth.
        const float3 highResolution = sceneColor.SampleLevel(sceneSampler, input.uv, 0.0f).rgb;
        return float4(lerp(highResolution, UpsampleBloom(input.uv), c_bloomScatter), 1.0f);
    }

    uint width;
    uint height;
    sceneColor.GetDimensions(width, height);

    const float2 texelSize = 1.0f / float2(width, height);
    // Overlapping 13-tap low-pass filter prevents gaps between bright subpixels.
    const float2 offsets[13] =
    {
        float2(-2.0f, -2.0f), float2(0.0f, -2.0f), float2(2.0f, -2.0f),
        float2(-2.0f, 0.0f),  float2(0.0f, 0.0f),  float2(2.0f, 0.0f),
        float2(-2.0f, 2.0f),  float2(0.0f, 2.0f),  float2(2.0f, 2.0f),
        float2(-1.0f, -1.0f), float2(1.0f, -1.0f), float2(-1.0f, 1.0f), float2(1.0f, 1.0f)
    };
    const float weights[13] =
    {
        0.03125f, 0.0625f, 0.03125f,
        0.0625f, 0.125f, 0.0625f,
        0.03125f, 0.0625f, 0.03125f,
        0.125f, 0.125f, 0.125f, 0.125f
    };

    float3 bloom = float3(0.0f, 0.0f, 0.0f);

    [unroll]
    for (uint index = 0; index < 13; ++index)
    {
        bloom += SampleBloomSource(input.uv + offsets[index] * texelSize) * weights[index];
    }

    return float4(bloom, 1.0f);
}

float3 LinearToSRGB(float3 color)
{
    return float3(
        color.r <= 0.0031308f ? color.r * 12.92f : 1.055f * pow(color.r, 1.0f / 2.4f) - 0.055f,
        color.g <= 0.0031308f ? color.g * 12.92f : 1.055f * pow(color.g, 1.0f / 2.4f) - 0.055f,
        color.b <= 0.0031308f ? color.b * 12.92f : 1.055f * pow(color.b, 1.0f / 2.4f) - 0.055f);
}

float4 PSComposite(VSOutput input) : SV_Target
{
    const float3 scene = sceneColor.SampleLevel(sceneSampler, input.uv, 0.0f).rgb;

    const float3 bloom = bloomColor.SampleLevel(bloomSampler, input.uv, 0.0f).rgb;

    const float3 color = ApplyDisplayGrade(scene + bloom * c_bloomIntensity);
    float3 encodedColor = LinearToSRGB(color);
    const float dither = frac(52.9829189f * frac(dot(input.position.xy, float2(0.06711056f, 0.00583715f)))) - 0.5f;
    encodedColor = saturate(encodedColor + dither / 255.0f);

    // Dither after tone mapping in output code values, including sky and bloom.
    if (c_encodeSRGB)
    {
        return float4(encodedColor, 1.0f);
    }

    float3 linearColor = float3(
        encodedColor.r <= 0.04045f ? encodedColor.r / 12.92f : pow((encodedColor.r + 0.055f) / 1.055f, 2.4f),
        encodedColor.g <= 0.04045f ? encodedColor.g / 12.92f : pow((encodedColor.g + 0.055f) / 1.055f, 2.4f),
        encodedColor.b <= 0.04045f ? encodedColor.b / 12.92f : pow((encodedColor.b + 0.055f) / 1.055f, 2.4f));
    return float4(linearColor, 1.0f);
}
