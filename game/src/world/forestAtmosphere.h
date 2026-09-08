#pragma once

#ifdef __cplusplus
namespace World
{
#define FOREST_CONSTANT inline constexpr float
#else
#define FOREST_CONSTANT static const float
#endif

// -----------------------------------------------------------------------------------------------------------------------------
// Environment
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_fogRed = 0.060f;
FOREST_CONSTANT c_fogGreen = 0.090f;
FOREST_CONSTANT c_fogBlue = 0.155f;

// Keep unlit geometry readable.
FOREST_CONSTANT c_ambientLightStrength = 0.74f;


// -----------------------------------------------------------------------------------------------------------------------------
// Moon
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_moonDirectionX = -0.55f;
FOREST_CONSTANT c_moonDirectionY = 0.40f;
FOREST_CONSTANT c_moonDirectionZ = -0.72f;

FOREST_CONSTANT c_moonRed = 0.36f;
FOREST_CONSTANT c_moonGreen = 0.43f;
FOREST_CONSTANT c_moonBlue = 0.55f;

FOREST_CONSTANT c_moonIntensity = 0.90f;
FOREST_CONSTANT c_moonRadiance = 2.0f;


// -----------------------------------------------------------------------------------------------------------------------------
// Sky / indirect light
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_skyRed = 0.020f;
FOREST_CONSTANT c_skyGreen = 0.032f;
FOREST_CONSTANT c_skyBlue = 0.055f;

FOREST_CONSTANT c_groundBounceRed = 0.050f;
FOREST_CONSTANT c_groundBounceGreen = 0.060f;
FOREST_CONSTANT c_groundBounceBlue = 0.055f;


// -----------------------------------------------------------------------------------------------------------------------------
// Ambient Occlusion
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_occlusionRadius = 1.4f;
FOREST_CONSTANT c_occlusionStrength = 0.70f;
FOREST_CONSTANT c_occlusionMinVisibility = 0.86f;



// -----------------------------------------------------------------------------------------------------------------------------
// Fog
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_fogStart = 18.0f;
FOREST_CONSTANT c_fogDensity = 0.020f;

FOREST_CONSTANT c_heightFogBaseHeight = 2.0f;
FOREST_CONSTANT c_heightFogFalloff = 0.16f;
FOREST_CONSTANT c_heightFogDensity = 0.016f;


// -----------------------------------------------------------------------------------------------------------------------------
// Display grading
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_sceneExposure = 1.08f;
FOREST_CONSTANT c_colorSaturation = 0.97f;
FOREST_CONSTANT c_colorContrast = 1.03f;
FOREST_CONSTANT c_displayGamma = 1.22f;


// -----------------------------------------------------------------------------------------------------------------------------
// Distance fading
// -----------------------------------------------------------------------------------------------------------------------------

FOREST_CONSTANT c_fogEdgeStart = 52.0f;
FOREST_CONSTANT c_fogEnd = 124.0f;

FOREST_CONSTANT c_detailFadeStart = 80.0f;
FOREST_CONSTANT c_detailFadeEnd = 112.0f;


#undef FOREST_CONSTANT

#ifndef __cplusplus

// -----------------------------------------------------------------------------------------------------------------------------
// Noise
// -----------------------------------------------------------------------------------------------------------------------------

float ForestHash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float ForestNoise(float2 p)
{
    float2 cell = floor(p);
    float2 f = frac(p);

    f = f * f * (3.0f - 2.0f * f);

    return lerp(
        lerp(ForestHash(cell), ForestHash(cell + float2(1, 0)), f.x),
        lerp(ForestHash(cell + float2(0, 1)), ForestHash(cell + 1.0f), f.x),
        f.y);
}


// -----------------------------------------------------------------------------------------------------------------------------
// Forest surface variation
// -----------------------------------------------------------------------------------------------------------------------------

void ApplyForestSurface(
    float3 worldPosition,
    float3 normal,
    uint flags,
    float metallic,
    float emissiveStrength,
    inout float3 albedo,
    inout float roughness)
{
    // Only explicitly tagged terrain/rocks participate;
    // characters and crystals keep their materials.
    if ((flags & 17u) == 0 || (flags & 8u) != 0 || metallic > 0.25f || emissiveStrength > 0.0f)
    {
        return;
    }

    float broad = ForestNoise(worldPosition.xz * 0.075f);
    float patches = ForestNoise(worldPosition.xz * 0.38f + 19.0f);
    float fine = ForestNoise(worldPosition.xz * 1.8f);

    // Suppress fine variation when its footprint becomes subpixel.
    float fineVisibility = 1.0f - saturate(length(fwidth(worldPosition.xz)) * 1.8f);

    // Slightly reduced dark-side variation so forest surfaces stay readable.
    float variation =
        lerp(0.91f, 1.08f, broad) *
        lerp(1.0f, lerp(0.96f, 1.04f, fine), fineVisibility);

    albedo *= variation;

    if ((flags & 1u) != 0)
    {
        float soil = smoothstep(0.38f, 0.72f, broad * 0.65f + patches * 0.35f);

        float damp =
            (1.0f - smoothstep(0.30f, 0.52f, broad)) *
            smoothstep(0.55f, 0.95f, normal.y);

        albedo = lerp(
            albedo,
            albedo * float3(1.06f, 0.84f, 0.70f),
            soil * 0.55f);

        // Previously 16%; that could make already dark terrain unnecessarily black.
        albedo *= 1.0f - damp * 0.08f;

        roughness = lerp(
            roughness,
            max(0.60f, roughness * 0.78f),
            damp);
    }
    else
    {
        float moss =
            smoothstep(0.30f, 0.82f, normal.y) *
            smoothstep(0.35f, 0.70f, patches);

        float luminance = dot(
            albedo,
            float3(0.2126f, 0.7152f, 0.0722f));

        // Slightly brighter moss than before.
        albedo = lerp(
            albedo,
            luminance * float3(0.58f, 0.78f, 0.38f),
            moss * 0.55f);

        roughness = lerp(roughness, 0.94f, moss);
    }
}

#endif

#ifdef __cplusplus
}
#endif