#pragma once

// Planned concentric progression: Forest, Desert, Ice, Lava.
namespace World
{
    constexpr int c_chunkSize = 32;

    constexpr int c_worldChunkCountX = 192;
    constexpr int c_worldChunkCountZ = 192;
    constexpr int c_chunkLoadRadius  = 4;

    constexpr float c_forestRadius  = 2800.0f;
    constexpr float c_dungeonRadius = 96.0f;

    constexpr int c_forestWallCount = 4480;
}
