#pragma once

#include "math/vector3.h"
#include "worldConfig.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace World
{
    struct sForestDungeon;

    namespace sMushroomDirection
    {
        enum Enum : uint8_t
        {
            North,
            East,
            South,
            West,
            Undefined
        };
    }

    namespace sMushroomModuleType
    {
        enum Enum : uint8_t
        {
            Entrance,
            Corridor,
            Turn,
            SmallCombat,
            LargeCombat,
            SideRoom,
            VerticalTransition,
            BossArena,
            Nursery,
            Archive,
            Alchemy,
            Shrine,
            Grotto,
            ElderShell,
            BossApproach,
            Count
        };
    }

    namespace sMushroomModuleCategory
    {
        enum Enum : uint8_t
        {
            Entrance,
            Corridor,
            CombatSmall,
            CombatLarge,
            Optional,
            Vertical,
            Boss,
            Landmark
        };
    }

    struct sMushroomDungeonModuleDesc
    {
        const char* prefabPath = nullptr;
        Engine::Math::cVec3f footprint = { 1.0f, 1.0f, 1.0f };
        sMushroomDirection::Enum inputDirection = sMushroomDirection::South;
        sMushroomDirection::Enum outputDirection = sMushroomDirection::North;
        uint8_t rotationMask = 0;
        uint8_t weight = 1;
        sMushroomModuleCategory::Enum category = sMushroomModuleCategory::Corridor;
        // Authored local sockets. Flexible shells seal every unused socket.
        uint8_t socketMask = 0x0f;
        bool sealUnusedSockets = true;
        uint8_t upperSocketMask = 0;
    };

    struct sMushroomDungeonModule
    {
        sMushroomModuleType::Enum type = sMushroomModuleType::Corridor;
        sMushroomModuleCategory::Enum category = sMushroomModuleCategory::Corridor;
        int floorIndex = 0;
        bool mainPath = false;
        int branchId = -1;
        // Indices in layout.modules; vertical upper connection has its own elevation.
        std::array<int, 4> neighbors = { -1, -1, -1, -1 };
        int upperNeighbor = -1;
        uint8_t upperConnectionMask = 0;
        uint8_t exteriorConnectionMask = 0;
        int cellX = 0;
        int cellZ = 0;
        float floorHeight = 0.0f;
        uint8_t rotationQuarterTurns = 0;
        uint8_t decorationVariant = 0;
        // World-space doorway bits: North, East, South, West. Closed sockets receive a wall.
        uint8_t connectionMask = 0;
        sMushroomDirection::Enum inputDirection = sMushroomDirection::South;
        sMushroomDirection::Enum outputDirection = sMushroomDirection::North;
    };

    struct sMushroomDungeonLayout
    {
        uint32_t seed = 0;
        std::vector<sMushroomDungeonModule> modules;
        int branchCount = 0;
        int loopCount = 0;
        int bossDistance = 0;
        int generationAttempt = 0;
        Engine::Math::cVec3f minimumBounds;
        Engine::Math::cVec3f maximumBounds;
    };

    constexpr float c_mushroomDungeonCellSize = 48.0f * c_mushroomDungeonScale;
    constexpr float c_mushroomDungeonFloorSpacing = 78.0f * c_mushroomDungeonScale;
    constexpr int c_mushroomDungeonFloorCount = 4;
    constexpr int c_mushroomDungeonMinimumBossDistance = 24;

    sMushroomDirection::Enum RotateDirection(sMushroomDirection::Enum _direction, uint8_t _quarterTurns);
    uint8_t RotateConnectionMask(uint8_t _mask, uint8_t _quarterTurns);
    bool CanModuleSatisfyConnections(sMushroomModuleType::Enum _type, uint8_t _connections,
        uint8_t _upperConnections, uint8_t _quarterTurns);
    bool ValidateMushroomDungeonLayout(const sMushroomDungeonLayout& _rLayout, std::string* _pError = nullptr);
    std::string DescribeMushroomDungeonLayout(const sMushroomDungeonLayout& _rLayout);

    const sMushroomDungeonModuleDesc& GetMushroomDungeonModuleDesc(sMushroomModuleType::Enum _type);
    void GenerateMushroomDungeonLayout(int _worldSeed, sForestDungeon& _rDungeon);
}
