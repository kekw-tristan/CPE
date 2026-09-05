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

        // Keep the starting area, routes and ruins level; blend into the wilderness.
        const float radius = sqrt(_x * _x + _z * _z);
        const float blend = (radius - 140.0f) / 260.0f;
        const float t = blend < 0.0f ? 0.0f : (blend > 1.0f ? 1.0f : blend);
        const float envelope = t * t * (3.0f - 2.0f * t);

        const float ridges = sin(_x * 0.006f + sin(_z * 0.004f) * 1.4f);
        const float valleys = cos(_z * 0.007f - _x * 0.002f);
        const float hills = sin(_x * 0.018f + _z * 0.011f) * cos(_z * 0.014f);

        return envelope * (ridges * 38.0f + valleys * 24.0f + hills * 6.0f);
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
            return a * (1.0f - v) + GetTerrainHeight(x, z + 1.0f) * (v - u) + b * u;

        return a * (1.0f - u) + GetTerrainHeight(x + 1.0f, z) * (u - v) + b * v;
    }
}
#endif
