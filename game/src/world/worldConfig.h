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

    // Mushroom kingdom envelope, including its peripheral rooms and bridges.
    constexpr float c_mushroomDungeonRadius = 450.0f;
    constexpr float c_mushroomDungeonHalfWidth = 224.0f;
    constexpr float c_mushroomDungeonFront = -140.0f;
    constexpr float c_mushroomDungeonBack = 232.0f;

    constexpr float c_bossDungeonHalfWidth = 52.0f;
    constexpr float c_bossDungeonFront = -108.0f;
    constexpr float c_bossDungeonBack = 44.0f;
    constexpr float c_bossDungeonApproach = -164.0f;

    // Royal aviary envelope, including the exterior crown approach.
    constexpr float c_cageDungeonRadius = 520.0f;
    constexpr float c_cageDungeonHalfWidth = 150.0f;
    constexpr float c_cageDungeonFront = -276.0f;
    constexpr float c_cageDungeonBack = 80.0f;
    constexpr float c_cageDungeonApproach = -316.0f;

    constexpr int c_forestWallCount = 4480;
}
