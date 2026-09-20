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

    // Sporecap keeps its progression distance; its streamed envelope comes from the generated module bounds.
    constexpr float c_sporecapDungeonRadius = 450.0f;
    // Shared by prefab placement, room spacing and encounter bounds.
    constexpr float c_mushroomDungeonScale = 0.44f;

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

    // Herzholz includes the root caverns and the outer canopy branches.
    constexpr float c_treeDungeonRadius = 620.0f;
    constexpr float c_treeDungeonHalfWidth = 190.0f;
    constexpr float c_treeDungeonBack = 155.0f;

    // Bernsteinkern: fractured shell, root road and traversable cap.
    constexpr float c_acornDungeonRadius = 720.0f;
    constexpr float c_acornDungeonHalfWidth = 184.0f;
    constexpr float c_acornDungeonFront = -252.0f;
    constexpr float c_acornDungeonBack = 184.0f;
    constexpr float c_acornDungeonApproach = -300.0f;

    constexpr int c_forestWallCount = 4480;
}
