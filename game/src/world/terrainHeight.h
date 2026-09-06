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

        const float radius = sqrt(_x * _x + _z * _z);

        // -------------------------------------------------------------------------------------------------------------------------
        // Spawn blend
        // -------------------------------------------------------------------------------------------------------------------------

        const float blend = (radius - 35.0f) / 100.0f;
        const float t = blend < 0.0f ? 0.0f : (blend > 1.0f ? 1.0f : blend);
        const float envelope = t * t * (3.0f - 2.0f * t);

        // -------------------------------------------------------------------------------------------------------------------------
        // Large mountains
        // -------------------------------------------------------------------------------------------------------------------------

        const float mountainA =
            sin(_x * 0.0045f + _z * 0.0025f)
            * cos(_z * 0.0035f)
            * 10.0f;

        const float mountainB =
            sin(_x * 0.0070f - _z * 0.0050f + 1.7f)
            * 6.0f;

        // -------------------------------------------------------------------------------------------------------------------------
        // Rolling hills
        // -------------------------------------------------------------------------------------------------------------------------

        const float hills =
            sin(_x * 0.0140f + _z * 0.0100f)
            * cos(_z * 0.0110f - _x * 0.0040f)
            * 4.0f;

        // -------------------------------------------------------------------------------------------------------------------------
        // Small terrain variation
        // -------------------------------------------------------------------------------------------------------------------------

        const float detail =
            sin(_x * 0.0310f + _z * 0.0270f)
            * 1.25f;

        // -------------------------------------------------------------------------------------------------------------------------
        // Mountain belt
        // -------------------------------------------------------------------------------------------------------------------------

        const float mountainRing =
            sin(radius * 0.018f - 1.2f)
            * 5.0f;

        // -------------------------------------------------------------------------------------------------------------------------

        return envelope * (mountainA + mountainB + hills + detail + mountainRing);
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