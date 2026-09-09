#pragma once

// Shared by world generation and all terrain vertex shaders.
#ifdef __cplusplus
#include <cmath>

namespace World
{
#endif

    inline float GetTerrainHeight(float _x, float _z)
    {
#ifdef __cplusplus
        using std::sin;
        using std::cos;
        using std::sqrt;
#endif

        // Warped ridges break up the skyline without a repeating concentric ring.
        const float warpedX = _x + sin(_z * 0.006f) * 42.0f;
        const float warpedZ = _z + sin(_x * 0.004f + 1.3f) * 55.0f;
        const float ridge = sin(warpedX * 0.008f + warpedZ * 0.003f);
        const float mountains = ridge * ridge * 62.0f
            + sin(warpedZ * 0.005f - warpedX * 0.002f) * 23.0f;

        // A winding, broad valley separates the mountain shoulders.
        const float valleyAxis = _x - sin(_z * 0.005f) * 105.0f;
        const float valley = 1.0f / (1.0f + valleyAxis * valleyAxis * 0.00012f);
        const float foothills = sin(warpedX * 0.019f) * cos(warpedZ * 0.015f) * 5.0f;
        const float detail = sin(_x * 0.043f + _z * 0.026f) * 0.65f;
        const float landscape = mountains * (1.0f - valley * 0.65f)
            - valley * 20.0f + foothills + detail;

        // The entire sanctuary and its stairs sit on a level overlook.
        // A long smooth shoulder provides walkable descents into the valleys.
        const float spawnZ = _z - 34.0f;
        const float radius = sqrt(_x * _x + spawnZ * spawnZ);
        const float blend = (radius - 90.0f) / 180.0f;
        const float t = blend < 0.0f ? 0.0f : (blend > 1.0f ? 1.0f : blend);
        const float envelope = t * t * (3.0f - 2.0f * t);

        return 48.0f + envelope * (landscape - 48.0f);
    }

#ifdef __cplusplus

    // Match the one-unit triangles of ChunkPlane, including their diagonal.
    // -------------------------------------------------------------------------------------------------------------------------

    inline float GetTerrainSurfaceHeight(float _x, float _z)
    {
        const float x = std::floor(_x);
        const float z = std::floor(_z);

        const float u = _x - x;
        const float v = _z - z;

        const float a = GetTerrainHeight(x, z);
        const float b = GetTerrainHeight(x + 1.0f, z + 1.0f);

        if (v >= u)
        {
            return a * (1.0f - v)
                + GetTerrainHeight(x, z + 1.0f) * (v - u)
                + b * u;
        }

        return a * (1.0f - u)
            + GetTerrainHeight(x + 1.0f, z) * (u - v)
            + b * v;
    }

}

#endif