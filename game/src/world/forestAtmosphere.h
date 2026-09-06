#pragma once

#ifdef __cplusplus
namespace World
{
#define FOREST_CONSTANT inline constexpr float
#else
#define FOREST_CONSTANT static const float
#endif

// Display-linear fog color, also used as the application's background color.
FOREST_CONSTANT c_fogRed = 0.035f;
FOREST_CONSTANT c_fogGreen = 0.055f;
FOREST_CONSTANT c_fogBlue = 0.095f;
FOREST_CONSTANT c_ambientLightStrength = 0.32f;
FOREST_CONSTANT c_fogStart = 18.0f;
FOREST_CONSTANT c_fogDensity = 0.018f;
// Fully opaque before the closest loaded edge (128 units), allowing for camera offset.
FOREST_CONSTANT c_fogEdgeStart = 52.0f;
FOREST_CONSTANT c_fogEnd = 124.0f;
FOREST_CONSTANT c_detailFadeStart = 80.0f;
FOREST_CONSTANT c_detailFadeEnd = 112.0f;

#undef FOREST_CONSTANT
#ifdef __cplusplus
}
#endif
