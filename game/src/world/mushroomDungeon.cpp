#include "mushroomDungeon.h"

#include "chunk.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{
    namespace
    {
        constexpr std::array<int, 4> c_stepX = { 0, 1, 0, -1 };
        constexpr std::array<int, 4> c_stepZ = { 1, 0, -1, 0 };
        constexpr int c_maximumAttempts = 32;

        constexpr std::array<sMushroomDungeonModuleDesc, sMushroomModuleType::Count> c_moduleDescs =
        {
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/entrance.prefab.json",            { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x01, 1, sMushroomModuleCategory::Entrance },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/corridor.prefab.json",            { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 3, sMushroomModuleCategory::Corridor },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/turn.prefab.json",                { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::East,      0x0f, 2, sMushroomModuleCategory::Corridor },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/small_combat.prefab.json",        { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 4, sMushroomModuleCategory::CombatSmall },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/large_combat.prefab.json",        { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 3, sMushroomModuleCategory::CombatLarge },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/side_room.prefab.json",           { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::Undefined, 0x0f, 1, sMushroomModuleCategory::Optional },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/vertical_transition.prefab.json", { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 1, sMushroomModuleCategory::Vertical, 0x04, false, 0x01 },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/boss_arena.prefab.json",          { 76.0f / 48.0f, 1.0f, 76.0f / 48.0f }, sMushroomDirection::South,     sMushroomDirection::Undefined, 0x01, 1, sMushroomModuleCategory::Boss, 0x04, false },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/nursery.prefab.json",             { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 3, sMushroomModuleCategory::CombatSmall },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/archive.prefab.json",             { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 2, sMushroomModuleCategory::Optional },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/alchemy.prefab.json",             { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 2, sMushroomModuleCategory::CombatSmall },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/shrine.prefab.json",              { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 1, sMushroomModuleCategory::Optional },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/grotto.prefab.json",              { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South,     sMushroomDirection::North,     0x0f, 3, sMushroomModuleCategory::CombatLarge },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/elder_shell.prefab.json",         { 17.5f, 1.0f, 17.5f }, sMushroomDirection::Undefined, sMushroomDirection::Undefined, 0x01, 1, sMushroomModuleCategory::Landmark, 0x00, false },
            sMushroomDungeonModuleDesc{ "./assets/prefabs/mushroom/boss_approach.prefab.json",       { 1.0f, 1.0f, 1.0f }, sMushroomDirection::South, sMushroomDirection::North, 0x01, 1, sMushroomModuleCategory::Corridor, 0x05, false }
        };

        struct sDungeonNode
        {
            int x = 0;
            int z = 0;
            int floor = 0;
            bool mainPath = true;
            int branch = -1;
            sMushroomModuleType::Enum type = sMushroomModuleType::Corridor;
            std::array<int, 4> neighbors = { -1, -1, -1, -1 };
            int upperNeighbor = -1;
            int upwardDirection = -1;
        };

        struct sDungeonGraph
        {
            std::vector<sDungeonNode> nodes;
            int branches = 0;
            int loops = 0;
        };

        struct sFloorGenerationSettings
        {
            int minimumBranchRooms;
            int maximumBranchRooms;
            int combatChance;
            int largeCombatChance;
            int loopChance;
            sMushroomModuleType::Enum theme;
        };

        constexpr std::array<sFloorGenerationSettings, 4> c_floorSettings =
        {{
            { 4, 6, 55, 0, 12, sMushroomModuleType::Nursery },
            { 4, 6, 65, 12, 25, sMushroomModuleType::Alchemy },
            { 4, 6, 75, 25, 45, sMushroomModuleType::Grotto },
            { 2, 3, 80, 30, 10, sMushroomModuleType::SmallCombat }
        }};

        // -------------------------------------------------------------------------------------------------------------------------

        std::array<int, 4> RandomDirections(std::mt19937& _rRandom)
        {
            std::array<int, 4> directions = { 0, 1, 2, 3 };
            // Explicit shuffle keeps results independent of the STL shuffle implementation.
            for (int i = 3; i > 0; --i)
                std::swap(directions[i], directions[_rRandom() % (i + 1)]);
            return directions;
        }

        bool IsFree(const sDungeonGraph& _rGraph, int _x, int _z, int _floor)
        {
            if (_floor < 0 || _floor >= c_mushroomDungeonFloorCount || _x * _x + _z * _z > 16)
                return false;
            // Keep the cathedral's central shaft open on every floor.
            // Only the explicitly placed arena and its bridge enter this volume.
            if (std::abs(_x) <= 1 && std::abs(_z) <= 1)
                return false;
            for (const auto& node : _rGraph.nodes)
            {
                if (node.x == _x && node.z == _z && (node.floor == _floor
                    || (node.type == sMushroomModuleType::VerticalTransition && node.floor + 1 == _floor)))
                    return false;
            }
            return true;
        }

        int AddNode(sDungeonGraph& _rGraph, int _x, int _z, int _floor, bool _mainPath = true, int _branch = -1)
        {
            sDungeonNode node;
            node.x = _x;
            node.z = _z;
            node.floor = _floor;
            node.mainPath = _mainPath;
            node.branch = _branch;
            _rGraph.nodes.push_back(node);
            return static_cast<int>(_rGraph.nodes.size()) - 1;
        }

        void Connect(sDungeonGraph& _rGraph, int _a, int _b)
        {
            auto& a = _rGraph.nodes[_a];
            auto& b = _rGraph.nodes[_b];
            for (int d = 0; d < 4; ++d)
            {
                if (b.x != a.x + c_stepX[d] || b.z != a.z + c_stepZ[d])
                    continue;
                if (b.floor != a.floor)
                {
                    a.upperNeighbor = _b;
                    a.upwardDirection = d;
                }
                else
                    a.neighbors[d] = _b;
                b.neighbors[(d + 2) % 4] = _a;
                return;
            }
        }

        // Bounded backtracking constructs a route before any branches exist.
        bool ExtendMainPath(sDungeonGraph& _rGraph, int _remaining, std::mt19937& _rRandom, int& _rBudget)
        {
            if (--_rBudget <= 0)
                return false;
            const int current = static_cast<int>(_rGraph.nodes.size()) - 1;
            const auto node = _rGraph.nodes[current];
            if (_remaining == 0)
            {
                if (node.floor == 3)
                    return node.x == 0 && node.z == -2;
                int input = -1;
                for (int d = 0; d < 4; ++d)
                    if (node.neighbors[d] >= 0)
                        input = d;
                const int output = (input + 2) % 4;
                if (input < 0 || !IsFree(_rGraph, node.x, node.z, node.floor + 1)
                    || !IsFree(_rGraph, node.x + c_stepX[output], node.z + c_stepZ[output], node.floor + 1))
                    return false;
                _rGraph.nodes[current].type = sMushroomModuleType::VerticalTransition;
                _rGraph.nodes[current].upwardDirection = output;
                return true;
            }
            if (node.floor == 3 && std::abs(node.x) + std::abs(node.z + 2) > _remaining)
                return false;
            for (int d : RandomDirections(_rRandom))
            {
                const int x = node.x + c_stepX[d];
                const int z = node.z + c_stepZ[d];
                if (!IsFree(_rGraph, x, z, node.floor))
                    continue;
                const int next = AddNode(_rGraph, x, z, node.floor);
                Connect(_rGraph, current, next);
                if (ExtendMainPath(_rGraph, _remaining - 1, _rRandom, _rBudget))
                    return true;
                _rGraph.nodes.pop_back();
                _rGraph.nodes[current].neighbors[d] = -1;
            }
            return false;
        }

        bool GenerateMainPath(sDungeonGraph& _rGraph, std::mt19937& _rRandom, bool _fallback)
        {
            // Fixed routes are used only after bounded seeded attempts fail.
            const std::array<std::vector<std::pair<int, int>>, 4> fallback =
            {{
                {{0,-4}, {0,-3}, {1,-3}, {2,-3}, {2,-2}, {3,-2}, {3,-1}, {3,0}},
                {{3,1}, {2,1}, {2,2}, {1,2}, {0,2}, {-1,2}, {-2,2}, {-2,1}},
                {{-2,0}, {-3,0}, {-3,-1}, {-2,-1}, {-2,-2}, {-2,-3}, {-1,-3}},
                {{0,-3}, {1,-3}, {2,-3}, {2,-2}, {1,-2}, {0,-2}}
            }};
            int previousStair = -1;
            for (int floor = 0; floor < 4; ++floor)
            {
                int x = 0;
                int z = -4;
                if (previousStair >= 0)
                {
                    const auto& stair = _rGraph.nodes[previousStair];
                    x = stair.x + c_stepX[stair.upwardDirection];
                    z = stair.z + c_stepZ[stair.upwardDirection];
                }
                const int first = AddNode(_rGraph, x, z, floor);
                if (previousStair >= 0)
                    Connect(_rGraph, previousStair, first);
                if (_fallback)
                {
                    for (size_t i = 1; i < fallback[floor].size(); ++i)
                    {
                        const auto [nextX, nextZ] = fallback[floor][i];
                        const int next = AddNode(_rGraph, nextX, nextZ, floor);
                        Connect(_rGraph, next - 1, next);
                    }
                    if (floor < 3)
                    {
                        auto& stair = _rGraph.nodes.back();
                        stair.type = sMushroomModuleType::VerticalTransition;
                        for (int d = 0; d < 4; ++d)
                            if (stair.neighbors[d] >= 0)
                                stair.upwardDirection = (d + 2) % 4;
                    }
                }
                else
                {
                    int budget = 6000;
                    const int length = floor == 3 ? 6 + static_cast<int>(_rRandom() % 5)
                        : 5 + static_cast<int>(_rRandom() % 2);
                    if (!ExtendMainPath(_rGraph, length, _rRandom, budget))
                        return false;
                }
                previousStair = static_cast<int>(_rGraph.nodes.size()) - 1;
            }
            _rGraph.nodes.front().type = sMushroomModuleType::Entrance;
            _rGraph.nodes.back().type = sMushroomModuleType::LargeCombat;
            const int approach = AddNode(_rGraph, 0, -1, 3);
            _rGraph.nodes[approach].type = sMushroomModuleType::BossApproach;
            Connect(_rGraph, approach - 1, approach);
            const int boss = AddNode(_rGraph, 0, 0, 3);
            _rGraph.nodes[boss].type = sMushroomModuleType::BossArena;
            Connect(_rGraph, approach, boss);
            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateBranches(sDungeonGraph& _rGraph, std::mt19937& _rRandom, bool _fallback)
        {
            for (int floor = 0; floor < 4; ++floor)
            {
                const auto& settings = c_floorSettings[floor];
                int remaining = settings.minimumBranchRooms + static_cast<int>(_rRandom()
                    % (settings.maximumBranchRooms - settings.minimumBranchRooms + 1));
                if (_fallback)
                    remaining = settings.minimumBranchRooms;
                std::vector<int> roots;
                for (size_t i = 1; i < _rGraph.nodes.size(); ++i)
                {
                    const auto& node = _rGraph.nodes[i];
                    if (node.floor == floor && node.mainPath && node.type == sMushroomModuleType::Corridor)
                        roots.push_back(static_cast<int>(i));
                }
                for (int attempt = 0; attempt < 80 && remaining > 0 && !roots.empty(); ++attempt)
                {
                    int current = roots[_rRandom() % roots.size()];
                    const int length = std::min(remaining, 1 + static_cast<int>(_rRandom() % 4));
                    bool added = false;
                    for (int step = 0; step < length; ++step)
                    {
                        const auto node = _rGraph.nodes[current];
                        int next = -1;
                        for (int d : RandomDirections(_rRandom))
                        {
                            const int x = node.x + c_stepX[d];
                            const int z = node.z + c_stepZ[d];
                            if (!IsFree(_rGraph, x, z, floor))
                                continue;
                            next = AddNode(_rGraph, x, z, floor, false, _rGraph.branches);
                            Connect(_rGraph, current, next);
                            --remaining;
                            added = true;
                            break;
                        }
                        if (next < 0)
                            break;
                        current = next;
                    }
                    if (added)
                        ++_rGraph.branches;
                }
            }
        }

        int GraphDistance(const sDungeonGraph& _rGraph, int _start, int _end)
        {
            std::vector<int> distance(_rGraph.nodes.size(), -1);
            std::queue<int> pending;
            distance[_start] = 0;
            pending.push(_start);
            while (!pending.empty())
            {
                const int current = pending.front();
                pending.pop();
                if (current == _end)
                    return distance[current];
                const auto& node = _rGraph.nodes[current];
                const std::array<int, 5> neighbors = { node.neighbors[0], node.neighbors[1],
                    node.neighbors[2], node.neighbors[3], node.upperNeighbor };
                for (int next : neighbors)
                {
                    if (next >= 0 && distance[next] < 0)
                    {
                        distance[next] = distance[current] + 1;
                        pending.push(next);
                    }
                }
            }
            return -1;
        }

        void GenerateLoops(sDungeonGraph& _rGraph, std::mt19937& _rRandom)
        {
            const int boss = static_cast<int>(std::find_if(_rGraph.nodes.begin(), _rGraph.nodes.end(),
                [](const sDungeonNode& _rNode) { return _rNode.type == sMushroomModuleType::BossArena; }) - _rGraph.nodes.begin());
            for (size_t i = 0; i < _rGraph.nodes.size(); ++i)
            {
                const auto node = _rGraph.nodes[i];
                if (node.mainPath || static_cast<int>(_rRandom() % 100) >= c_floorSettings[node.floor].loopChance)
                    continue;
                for (size_t j = 1; j < _rGraph.nodes.size(); ++j)
                {
                    const auto other = _rGraph.nodes[j];
                    if (other.floor != node.floor || other.type != sMushroomModuleType::Corridor
                        || other.branch == node.branch || std::abs(node.x - other.x) + std::abs(node.z - other.z) != 1
                        || GraphDistance(_rGraph, static_cast<int>(i), static_cast<int>(j)) < 3)
                        continue;
                    const auto beforeA = _rGraph.nodes[i];
                    const auto beforeB = _rGraph.nodes[j];
                    Connect(_rGraph, static_cast<int>(i), static_cast<int>(j));
                    if (GraphDistance(_rGraph, 0, boss) >= c_mushroomDungeonMinimumBossDistance)
                    {
                        ++_rGraph.loops;
                        break;
                    }
                    _rGraph.nodes[i] = beforeA;
                    _rGraph.nodes[j] = beforeB;
                }
            }
        }

        void AssignRoomCategories(sDungeonGraph& _rGraph, std::mt19937& _rRandom)
        {
            std::array<int, 4> largeCounts{};
            int shrines = 0;
            int archives = 0;
            for (auto& node : _rGraph.nodes)
            {
                if (node.type != sMushroomModuleType::Corridor)
                    continue;
                const auto& settings = c_floorSettings[node.floor];
                const int degree = static_cast<int>(std::count_if(node.neighbors.begin(), node.neighbors.end(),
                    [](int _neighbor) { return _neighbor >= 0; }));
                if (!node.mainPath && degree == 1)
                {
                    const auto roll = _rRandom() % 100;
                    node.type = roll < 15 && shrines < 2 ? sMushroomModuleType::Shrine
                        : (roll < 40 && archives < 3 ? sMushroomModuleType::Archive : sMushroomModuleType::SideRoom);
                    shrines += node.type == sMushroomModuleType::Shrine;
                    archives += node.type == sMushroomModuleType::Archive;
                }
                else if (static_cast<int>(_rRandom() % 100) < settings.combatChance)
                {
                    node.type = _rRandom() % 100 < 35 ? settings.theme : sMushroomModuleType::SmallCombat;
                    if (node.floor > 0 && largeCounts[node.floor] < (node.floor == 3 ? 1 : 2)
                        && static_cast<int>(_rRandom() % 100) < settings.largeCombatChance)
                        node.type = sMushroomModuleType::LargeCombat;
                }
                else if (!node.mainPath)
                    node.type = sMushroomModuleType::SideRoom;
                else
                {
                    uint8_t mask = 0;
                    for (int d = 0; d < 4; ++d)
                        if (node.neighbors[d] >= 0)
                            mask |= static_cast<uint8_t>(1u << d);
                    node.type = degree == 2 && mask != 0x05 && mask != 0x0a
                        ? sMushroomModuleType::Turn : sMushroomModuleType::Corridor;
                }
                for (int neighbor : node.neighbors)
                {
                    if (neighbor >= 0 && _rGraph.nodes[neighbor].type == node.type
                        && node.type != sMushroomModuleType::Corridor && node.type != sMushroomModuleType::SmallCombat
                        && node.type != sMushroomModuleType::SideRoom)
                        node.type = sMushroomModuleType::SmallCombat;
                }
                if (GetMushroomDungeonModuleDesc(node.type).category == sMushroomModuleCategory::CombatLarge)
                {
                    if (largeCounts[node.floor] >= 2)
                        node.type = sMushroomModuleType::SmallCombat;
                    else
                        ++largeCounts[node.floor];
                }
            }
        }

        sMushroomDungeonLayout BuildMushroomDungeonLayout(const sDungeonGraph& _rGraph, uint32_t _seed,
            int _attempt, std::mt19937& _rRandom)
        {
            sMushroomDungeonLayout layout;
            layout.seed = _seed;
            layout.branchCount = _rGraph.branches;
            layout.loopCount = _rGraph.loops;
            layout.generationAttempt = _attempt;
            // Includes asymmetrical cap, roots and the exterior staircase.
            layout.minimumBounds = Engine::Math::cVec3f(-420.0f, -150.0f, -448.0f) * c_mushroomDungeonScale;
            layout.maximumBounds = Engine::Math::cVec3f(420.0f, 350.0f, 420.0f) * c_mushroomDungeonScale;
            layout.modules.reserve(_rGraph.nodes.size() + 1);
            for (const auto& node : _rGraph.nodes)
            {
                sMushroomDungeonModule module;
                module.type = node.type;
                module.category = GetMushroomDungeonModuleDesc(node.type).category;
                module.cellX = node.x;
                module.cellZ = node.z;
                module.floorIndex = node.floor;
                module.floorHeight = node.floor * c_mushroomDungeonFloorSpacing;
                module.mainPath = node.mainPath;
                module.branchId = node.branch;
                module.neighbors = node.neighbors;
                module.upperNeighbor = node.upperNeighbor;
                module.upperConnectionMask = node.upperNeighbor >= 0 ? static_cast<uint8_t>(1u << node.upwardDirection) : 0;
                module.exteriorConnectionMask = node.type == sMushroomModuleType::Entrance ? 0x04 : 0;
                module.connectionMask = module.exteriorConnectionMask;
                for (int d = 0; d < 4; ++d)
                    if (node.neighbors[d] >= 0)
                        module.connectionMask |= static_cast<uint8_t>(1u << d);
                // Orient authored input toward the previous main room / branch parent.
                const auto& desc = GetMushroomDungeonModuleDesc(node.type);
                int preferredInput = 2;
                int earliest = static_cast<int>(_rGraph.nodes.size());
                for (int d = 0; d < 4; ++d)
                {
                    if (node.neighbors[d] >= 0 && node.neighbors[d] < earliest)
                    {
                        earliest = node.neighbors[d];
                        preferredInput = d;
                    }
                }
                for (int offset = 0; offset < 4; ++offset)
                {
                    const auto rotation = static_cast<uint8_t>((preferredInput + 2 + offset) % 4);
                    if (CanModuleSatisfyConnections(node.type, module.connectionMask, module.upperConnectionMask, rotation))
                    {
                        module.rotationQuarterTurns = rotation;
                        break;
                    }
                }
                module.inputDirection = RotateDirection(desc.inputDirection, module.rotationQuarterTurns);
                module.outputDirection = RotateDirection(desc.outputDirection, module.rotationQuarterTurns);
                if ((module.connectionMask & (1u << module.inputDirection)) == 0)
                    module.inputDirection = sMushroomDirection::Undefined;
                if (((module.connectionMask | module.upperConnectionMask) & (1u << module.outputDirection)) == 0)
                    module.outputDirection = sMushroomDirection::Undefined;
                module.decorationVariant = static_cast<uint8_t>(_rRandom() % 10);
                layout.modules.push_back(module);
                if (node.type == sMushroomModuleType::BossArena)
                    layout.bossDistance = GraphDistance(_rGraph, 0, static_cast<int>(layout.modules.size()) - 1);
            }
            sMushroomDungeonModule shell;
            shell.type = sMushroomModuleType::ElderShell;
            shell.category = sMushroomModuleCategory::Landmark;
            shell.inputDirection = sMushroomDirection::Undefined;
            shell.outputDirection = sMushroomDirection::Undefined;
            layout.modules.push_back(shell);
            return layout;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMushroomDirection::Enum RotateDirection(sMushroomDirection::Enum _direction, uint8_t _quarterTurns)
    {
        return _direction == sMushroomDirection::Undefined ? _direction
            : static_cast<sMushroomDirection::Enum>((_direction + _quarterTurns) % 4);
    }

    uint8_t RotateConnectionMask(uint8_t _mask, uint8_t _quarterTurns)
    {
        uint8_t result = 0;
        for (int d = 0; d < 4; ++d)
            if ((_mask & (1u << d)) != 0)
                result |= static_cast<uint8_t>(1u << ((d + _quarterTurns) % 4));
        return result;
    }

    const sMushroomDungeonModuleDesc& GetMushroomDungeonModuleDesc(sMushroomModuleType::Enum _type)
    {
        return c_moduleDescs[static_cast<size_t>(_type)];
    }

    bool CanModuleSatisfyConnections(sMushroomModuleType::Enum _type, uint8_t _connections,
        uint8_t _upperConnections, uint8_t _quarterTurns)
    {
        if (_type >= sMushroomModuleType::Count || _quarterTurns >= 4)
            return false;
        const auto& desc = GetMushroomDungeonModuleDesc(_type);
        const auto sockets = RotateConnectionMask(desc.socketMask, _quarterTurns);
        return (desc.rotationMask & (1u << _quarterTurns)) != 0
            && (desc.sealUnusedSockets ? (_connections & ~sockets) == 0 : _connections == sockets)
            && _upperConnections == RotateConnectionMask(desc.upperSocketMask, _quarterTurns);
    }
    // -------------------------------------------------------------------------------------------------------------------------

    bool ValidateMushroomDungeonLayout(const sMushroomDungeonLayout& _rLayout, std::string* _pError)
    {
        const auto fail = [&](const std::string& _rMessage)
        {
            if (_pError)
                *_pError = _rMessage;
            return false;
        };
        const auto& modules = _rLayout.modules;
        int entrance = -1;
        int boss = -1;
        int stairs = 0;
        int landmarks = 0;
        int approaches = 0;
        int combat = 0;
        int optional = 0;
        std::array<int, 4> floorRooms{};
        std::array<int, 3> floorStairs{};
        for (size_t i = 0; i < modules.size(); ++i)
        {
            const auto& m = modules[i];
            const auto label = "Module " + std::to_string(i) + " (floor " + std::to_string(m.floorIndex)
                + ", " + std::to_string(m.cellX) + ", " + std::to_string(m.cellZ) + "): ";
            if (m.type >= sMushroomModuleType::Count || m.category != GetMushroomDungeonModuleDesc(m.type).category)
                return fail(label + "invalid type/category");
            if (!CanModuleSatisfyConnections(m.type, m.connectionMask, m.upperConnectionMask, m.rotationQuarterTurns))
                return fail(label + "rotation does not satisfy authored sockets");
            if (m.category == sMushroomModuleCategory::Landmark)
            {
                ++landmarks;
                if (m.upperNeighbor != -1 || std::any_of(m.neighbors.begin(), m.neighbors.end(), [](int _i) { return _i != -1; }))
                    return fail(label + "landmark has gameplay edges");
                continue;
            }
            if (m.floorIndex < 0 || m.floorIndex >= 4 || m.floorHeight != m.floorIndex * c_mushroomDungeonFloorSpacing)
                return fail(label + "invalid floor elevation");
            if (m.cellX * m.cellX + m.cellZ * m.cellZ > 16)
                return fail(label + "room lies outside the reserved stem interior");
            if (std::abs(m.cellX) <= 1 && std::abs(m.cellZ) <= 1
                && m.type != sMushroomModuleType::BossArena && m.type != sMushroomModuleType::BossApproach)
                return fail(label + "room obstructs the central cathedral shaft");
            if (m.inputDirection > sMushroomDirection::Undefined || m.outputDirection > sMushroomDirection::Undefined)
                return fail(label + "invalid input/output direction");
            if ((m.inputDirection != sMushroomDirection::Undefined
                    && (m.connectionMask & (1u << m.inputDirection)) == 0)
                || (m.outputDirection != sMushroomDirection::Undefined
                    && ((m.connectionMask | m.upperConnectionMask) & (1u << m.outputDirection)) == 0))
                return fail(label + "input/output direction names a sealed socket");
            ++floorRooms[m.floorIndex];
            if (m.type == sMushroomModuleType::BossApproach)
            {
                ++approaches;
                if (m.floorIndex != 3 || m.cellX != 0 || m.cellZ != -1)
                    return fail(label + "boss connector is outside its reserved gateway");
            }
            if (m.type == sMushroomModuleType::Entrance)
            {
                if (entrance >= 0 || m.floorIndex != 0 || m.cellX != 0 || m.cellZ != -4 || m.exteriorConnectionMask != 0x04)
                    return fail(label + "entrance must be unique and meet the exterior path");
                entrance = static_cast<int>(i);
            }
            else if (m.exteriorConnectionMask != 0)
                return fail(label + "only the entrance may open to the exterior");
            if (m.type == sMushroomModuleType::BossArena)
            {
                if (boss >= 0 || m.floorIndex != 3 || m.cellX != 0 || m.cellZ != 0
                    || m.floorHeight != GetBossArenaHeight(sBossId::ForestSporecap))
                    return fail(label + "boss must be unique and meet the encounter anchor");
                boss = static_cast<int>(i);
            }
            combat += m.category == sMushroomModuleCategory::CombatSmall || m.category == sMushroomModuleCategory::CombatLarge;
            optional += m.category == sMushroomModuleCategory::Optional;
            const bool vertical = m.type == sMushroomModuleType::VerticalTransition;
            if (vertical)
            {
                if (m.floorIndex >= 3 || m.upperNeighbor < 0 || m.upperNeighbor >= static_cast<int>(modules.size()))
                    return fail(label + "invalid vertical destination");
                ++stairs;
                ++floorStairs[m.floorIndex];
                const auto& next = modules[m.upperNeighbor];
                const auto output = RotateDirection(sMushroomDirection::North, m.rotationQuarterTurns);
                if (next.floorIndex != m.floorIndex + 1 || next.cellX != m.cellX + c_stepX[output]
                    || next.cellZ != m.cellZ + c_stepZ[output] || next.neighbors[(output + 2) % 4] != static_cast<int>(i))
                    return fail(label + "upper landing is not aligned and bidirectional");
            }
            else if (m.upperNeighbor != -1)
                return fail(label + "non-stair has a vertical edge");
            for (int d = 0; d < 4; ++d)
            {
                const int neighbor = m.neighbors[d];
                const bool exterior = (m.exteriorConnectionMask & (1u << d)) != 0;
                if (((m.connectionMask & (1u << d)) != 0) != (neighbor >= 0 || exterior)
                    || neighbor < -1 || neighbor >= static_cast<int>(modules.size()) || (exterior && neighbor >= 0))
                    return fail(label + "door points into empty space or disagrees with its edge");
                if (neighbor < 0)
                    continue;
                const auto& next = modules[neighbor];
                const bool upperLanding = next.type == sMushroomModuleType::VerticalTransition
                    && next.floorIndex + 1 == m.floorIndex && next.upperNeighbor == static_cast<int>(i);
                if (next.cellX != m.cellX + c_stepX[d] || next.cellZ != m.cellZ + c_stepZ[d]
                    || (!upperLanding && (next.floorIndex != m.floorIndex || next.neighbors[(d + 2) % 4] != static_cast<int>(i))))
                    return fail(label + "edge is not aligned and bidirectional");
            }
            // Conservative occupied volumes include both stair floors. The boss connector
            // ends at z=-38, exactly at the arena edge, instead of intersecting its floor.
            for (size_t j = 0; j < i; ++j)
            {
                const auto& other = modules[j];
                if (other.category == sMushroomModuleCategory::Landmark)
                    continue;
                const auto bounds = [](const sMushroomDungeonModule& _rModule)
                {
                    const float half = (_rModule.type == sMushroomModuleType::BossArena ? 38.0f : 24.0f) * c_mushroomDungeonScale;
                    const float height = (_rModule.type == sMushroomModuleType::VerticalTransition ? 92.0f : 28.0f) * c_mushroomDungeonScale;
                    const float x = _rModule.cellX * c_mushroomDungeonCellSize;
                    const float z = _rModule.cellZ * c_mushroomDungeonCellSize;
                    return std::array<float, 6>{ x - half, _rModule.floorHeight - 16.0f * c_mushroomDungeonScale, z - half,
                        x + half, _rModule.floorHeight + height,
                        z + (_rModule.type == sMushroomModuleType::BossApproach ? 10.0f * c_mushroomDungeonScale : half) };
                };
                const auto a = bounds(m);
                const auto b = bounds(other);
                constexpr float c_contactTolerance = 0.001f;
                if (a[0] < b[3] - c_contactTolerance && a[3] > b[0] + c_contactTolerance
                    && a[1] < b[4] - c_contactTolerance && a[4] > b[1] + c_contactTolerance
                    && a[2] < b[5] - c_contactTolerance && a[5] > b[2] + c_contactTolerance)
                    return fail(label + "occupied volume overlaps module " + std::to_string(j));
            }
        }
        if (entrance < 0 || boss < 0 || stairs != 3 || landmarks != 1 || approaches != 1 || std::any_of(floorStairs.begin(), floorStairs.end(), [](int _n) { return _n != 1; }))
            return fail("Expected one entrance, boss, approach, shell, and one transition per floor pair");
        if (modules.size() < 40 || modules.size() > 60 || combat < 10 || optional < 4
            || std::any_of(floorRooms.begin(), floorRooms.end(), [](int _n) { return _n < 8 || _n > 17; }))
            return fail("Insufficient room/encounter variety or invalid floor room budget");
        std::vector<int> distance(modules.size(), -1);
        std::queue<int> pending;
        distance[entrance] = 0;
        pending.push(entrance);
        while (!pending.empty())
        {
            const int current = pending.front();
            pending.pop();
            const auto& m = modules[current];
            const std::array<int, 5> neighbors = { m.neighbors[0], m.neighbors[1], m.neighbors[2], m.neighbors[3], m.upperNeighbor };
            for (int next : neighbors)
            {
                if (next >= 0 && distance[next] < 0)
                {
                    distance[next] = distance[current] + 1;
                    pending.push(next);
                }
            }
        }
        for (size_t i = 0; i < modules.size(); ++i)
            if (modules[i].category != sMushroomModuleCategory::Landmark && distance[i] < 0)
                return fail("Unreachable room " + std::to_string(i));
        if (distance[boss] < c_mushroomDungeonMinimumBossDistance || distance[boss] != _rLayout.bossDistance)
            return fail("Boss path is too short or cached graph distance is incorrect");
        if (_pError)
            _pError->clear();
        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    std::string DescribeMushroomDungeonLayout(const sMushroomDungeonLayout& _rLayout)
    {
        std::ostringstream output;
        const auto mainRooms = std::count_if(_rLayout.modules.begin(), _rLayout.modules.end(),
            [](const sMushroomDungeonModule& _rModule) { return _rModule.mainPath; });
        output << "Mushroom Dungeon\nSeed: " << _rLayout.seed << "  Attempt: " << _rLayout.generationAttempt
            << "\nRooms: " << _rLayout.modules.size() - 1 << "  Main path: " << mainRooms
            << "  Branches: " << _rLayout.branchCount << "  Loops: " << _rLayout.loopCount
            << "\nEntrance -> Boss graph distance: " << _rLayout.bossDistance << '\n';
        for (int floor = 0; floor < 4; ++floor)
        {
            std::array<std::string, 17> rows;
            for (auto& row : rows)
                row = std::string(17, ' ');
            for (int z = 0; z < 9; ++z)
                for (int x = 0; x < 9; ++x)
                    rows[z * 2][x * 2] = '.';
            for (const auto& m : _rLayout.modules)
            {
                if (m.category == sMushroomModuleCategory::Landmark)
                    continue;
                const bool upper = m.type == sMushroomModuleType::VerticalTransition && m.floorIndex + 1 == floor;
                if (m.floorIndex != floor && !upper)
                    continue;
                const int x = (m.cellX + 4) * 2;
                const int z = (4 - m.cellZ) * 2;
                char symbol = 'R';
                switch (m.category)
                {
                    case sMushroomModuleCategory::Entrance: symbol = 'E'; break;
                    case sMushroomModuleCategory::CombatSmall: symbol = 'C'; break;
                    case sMushroomModuleCategory::CombatLarge: symbol = 'L'; break;
                    case sMushroomModuleCategory::Optional: symbol = 'O'; break;
                    case sMushroomModuleCategory::Vertical: symbol = upper ? 'v' : 'V'; break;
                    case sMushroomModuleCategory::Boss: symbol = 'B'; break;
                    default: break;
                }
                if (m.type == sMushroomModuleType::Shrine || m.type == sMushroomModuleType::Archive)
                    symbol = 'S';
                if (m.type == sMushroomModuleType::BossApproach)
                    symbol = 'G';
                rows[z][x] = symbol;
                const auto mask = upper ? m.upperConnectionMask : m.connectionMask;
                for (int d = 0; d < 4; ++d)
                {
                    const int px = x + c_stepX[d];
                    const int pz = z - c_stepZ[d];
                    if ((mask & (1u << d)) != 0 && px >= 0 && px < 17 && pz >= 0 && pz < 17)
                        rows[pz][px] = d % 2 == 0 ? '|' : '-';
                }
            }
            output << "Floor " << floor << " (height " << floor * c_mushroomDungeonFloorSpacing << ")\n";
            for (const auto& row : rows)
                output << row << '\n';
        }
        output << "E entrance, C combat, L large combat, O optional, S special, V stair up, v landing, G boss approach, B boss\n";
        return output.str();
    }

    void GenerateMushroomDungeonLayout(int _worldSeed, sForestDungeon& _rDungeon)
    {
        std::seed_seq seed{ static_cast<uint32_t>(_worldSeed), static_cast<uint32_t>(_rDungeon.bossId), 0x53504f52u };
        std::mt19937 seedGenerator(seed);
        const uint32_t dungeonSeed = seedGenerator();
        std::string error;
        for (int attempt = 0; attempt <= c_maximumAttempts; ++attempt)
        {
            const bool fallback = attempt == c_maximumAttempts;
            std::seed_seq attemptSeed{ dungeonSeed, static_cast<uint32_t>(attempt), 0x47524150u };
            std::mt19937 random(attemptSeed);
            // A reviewed fixed fallback is independent of the failed random attempts.
            if (fallback)
                random.seed(0x46414c4cu);
            sDungeonGraph graph;
            graph.nodes.reserve(64);
            if (!GenerateMainPath(graph, random, fallback))
                continue;
            GenerateBranches(graph, random, fallback);
            if (!fallback)
                GenerateLoops(graph, random);
            AssignRoomCategories(graph, random);
            auto layout = BuildMushroomDungeonLayout(graph, dungeonSeed, attempt, random);
            if (ValidateMushroomDungeonLayout(layout, &error))
            {
                _rDungeon.mushroomLayout = std::move(layout);
#if defined(GAME_DEBUG) && defined(MUSHROOM_DUNGEON_VERBOSE)
                std::cout << DescribeMushroomDungeonLayout(_rDungeon.mushroomLayout);
#endif
                return;
            }
#if defined(GAME_DEBUG)
            std::cerr << "Mushroom Dungeon seed " << dungeonSeed << ", attempt " << attempt << ": " << error << '\n';
#endif
        }
        throw std::runtime_error("Mushroom Dungeon deterministic fallback failed validation: " + error);
    }
}
