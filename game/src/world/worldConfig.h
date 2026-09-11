#pragma once

// Planned concentric progression: Forest, Desert, Ice, Lava.
namespace World
{
    constexpr int c_chunkSize = 64;

    constexpr int c_worldChunkCountX = 192;
    constexpr int c_worldChunkCountZ = 192;

    constexpr int c_chunkLoadRadius  = 5;

    constexpr float c_forestRadius  = 2800.0f;
    // Keep all four ruins and their guarded approaches outside the spawn clearing.
    constexpr float c_dungeonRadius = 190.0f;

    constexpr int c_forestWallCount = 4480;
}
