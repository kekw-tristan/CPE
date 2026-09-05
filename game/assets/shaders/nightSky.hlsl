#pragma once

#include "../../src/world/forestAtmosphere.h"

// All features are anchored to view direction, never to camera translation or cube UVs.
float SkyHash(float2 p)
{
    float3 q = frac(float3(p.x, p.y, p.x) * 0.1031f);
    q += dot(q, q.yzx + 33.33f);
    return frac((q.x + q.y) * q.z);
}

// -----------------------------------------------------------------------------------------------------------------------------

float SkySegment(float2 p, float2 a, float2 b)
{
    float2 edge = b - a;
    return length(p - a - edge * saturate(dot(p - a, edge) / dot(edge, edge)));
}

// -----------------------------------------------------------------------------------------------------------------------------

float SkyConstellation(float2 p)
{
    // A broken crystal outline with an off-center triangular spine.
    float segmentDistance = SkySegment(p, float2(0.0f, 0.085f), float2(-0.055f, 0.0f));
    
    segmentDistance = min(segmentDistance, SkySegment(p, float2(-0.055f, 0.0f), float2(0.0f, -0.065f)));
    segmentDistance = min(segmentDistance, SkySegment(p, float2(0.0f, -0.065f), float2(0.060f, 0.018f)));
    segmentDistance = min(segmentDistance, SkySegment(p, float2(0.060f, 0.018f), float2(0.0f, 0.085f)));
    segmentDistance = min(segmentDistance, SkySegment(p, float2(-0.055f, 0.0f), float2(0.025f, 0.030f)));
    segmentDistance = min(segmentDistance, SkySegment(p, float2(0.025f, 0.030f), float2(0.0f, -0.065f)));
    
    float aa        = max(length(fwidth(p)), 0.0002f);
    float outline   = 1.0f - smoothstep(0.00035f, 0.00035f + aa, segmentDistance);
    float nodes     = min(length(p - float2(0.0f, 0.085f)), length(p - float2(-0.055f, 0.0f)));
    
    nodes = min(nodes, length(p - float2(0.0f, -0.065f)));
    nodes = min(nodes, length(p - float2(0.060f, 0.018f)));
    
    return outline * 0.18f + (1.0f - smoothstep(0.0010f, 0.0025f + aa, nodes));
}

// -----------------------------------------------------------------------------------------------------------------------------

float3 SkyStars(float2 uv, float scale)
{
    float2 grid     = uv * float2(scale, scale * 0.5f);
    float2 cell     = floor(grid);
    float  seed     = SkyHash(cell);
    float2 center   = 0.2f + 0.6f * float2(SkyHash(cell + 17.0f), SkyHash(cell + 43.0f));
    float2 p        = frac(grid) - center;
    float  radius   = lerp(0.035f, 0.11f, seed * seed);
    float  aa       = max(length(fwidth(grid)), 0.015f);
    
    // Rhombi and four-point shards echo the crystals and pyramids in the world.
    
    float diamond   = abs(p.x) + abs(p.y);
    float cross     = min(max(abs(p.x) * 0.5f, abs(p.y) * 2.0f), max(abs(p.x) * 2.0f, abs(p.y) * 0.5f));
    float shape     = seed > 0.96f ? cross : diamond;
    float star      = (1.0f - smoothstep(radius, radius + aa, shape)) * step(0.78f, seed);
    star *= min(1.0f, radius / aa);
    return lerp(float3(0.30f, 0.55f, 0.65f), float3(0.85f, 0.72f, 0.48f), seed) * star;
}

// -----------------------------------------------------------------------------------------------------------------------------

float3 EvaluateNightSky(float3 direction)
{
    direction = normalize(direction);
    
    const float3 fogColor   = float3(c_fogRed, c_fogGreen, c_fogBlue);
    float        elevation  = saturate(direction.y);
    float        visibility = smoothstep(0.02f, 0.38f, elevation);
    float3       color      = lerp(float3(0.018f, 0.028f, 0.045f), float3(0.003f, 0.006f, 0.018f), elevation);

    // Angular, layered aurora ribbons rather than photographic clouds.
    float ribbonAxis    = dot(direction, normalize(float3(0.3f, 1.0f, -0.25f)));
    float folds         = abs(frac(dot(direction, float3(1.7f, 0.0f, 2.2f))) * 2.0f - 1.0f);
    float ribbon        = exp(-abs(ribbonAxis - 0.58f - folds * 0.06f) * 65.0f);
    float echo          = exp(-abs(ribbonAxis - 0.65f - folds * 0.035f) * 100.0f);
    
    color += ribbon * float3(0.010f, 0.042f, 0.036f) + echo * float3(0.021f, 0.012f, 0.039f);

    float2 uv = float2(atan2(direction.z, direction.x) / 6.2831853f + 0.5f, asin(clamp(direction.y, -1.0f, 1.0f)) / 3.14159265f + 0.5f);
    color += SkyStars(uv, 220.0f) * (1.0f - smoothstep(0.96f, 1.0f, elevation));

    float2 crystalCoordinates = float2(dot(direction, normalize(float3(0.8f, 0.0f, -0.6f))), dot(direction, normalize(float3(-0.27f, 0.89f, -0.36f))));
    
    if (dot(direction, normalize(float3(0.54f, 0.45f, 0.72f))) > 0.9f)
        color += SkyConstellation(crystalCoordinates) * float3(0.23f, 0.55f, 0.57f);

    float2 secondCoordinates = float2(dot(direction, normalize(float3(-0.7f, 0.0f, -0.7f))),
        dot(direction, normalize(float3(0.3f, 0.9f, -0.3f))));
    
    if (dot(direction, normalize(float3(-0.64f, 0.42f, 0.64f))) > 0.9f)
        color += SkyConstellation(secondCoordinates * 1.3f) * float3(0.43f, 0.30f, 0.57f);

    // A faceted diamond moon, split into four differently lit planes and a detached shard.
    float3 moonAxis     = normalize(float3(-0.55f, 0.40f, -0.72f));
    float3 moonRight    = normalize(cross(float3(0.0f, 1.0f, 0.0f), moonAxis));
    float3 moonUp       = cross(moonAxis, moonRight);
    float2 moon         = float2(dot(direction, moonRight), dot(direction, moonUp));
    if (dot(direction, moonAxis) > 0.95f)
    {
        float aa        = max(length(fwidth(moon)), 0.0002f);
        float diamond   = abs(moon.x) + abs(moon.y * 0.75f);
        float mask      = 1.0f - smoothstep(0.040f, 0.040f + aa, diamond);
        float facet     = 0.22f + 0.25f * step(0.0f, moon.x) + 0.26f * step(0.0f, moon.y);
        color           += mask * facet * float3(0.48f, 0.68f, 0.70f);
        float shard     = abs(moon.x - 0.054f) + abs((moon.y + 0.030f) * 0.7f);
        color           += (1.0f - smoothstep(0.009f, 0.009f + aa, shard)) * float3(0.13f, 0.25f, 0.27f);
    }

    return lerp(fogColor, color, visibility);
}
