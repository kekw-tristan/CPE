// Standalone CPU audit. Including the implementation also exercises the bounded fallback.
// Build with MSVC: cl /std:c++20 /EHsc /O2 /Iengine/src scripts/check_mushroom_layout.cpp
#include "../game/src/world/mushroomDungeon.cpp"

#include <limits>
#include <set>

namespace
{
    void Require(bool _condition, const std::string& _rMessage)
    {
        if (!_condition)
            throw std::runtime_error(_rMessage);
    }

    std::string Fingerprint(const World::sMushroomDungeonLayout& _rLayout)
    {
        std::ostringstream result;
        result << World::DescribeMushroomDungeonLayout(_rLayout);
        for (const auto& m : _rLayout.modules)
        {
            result << int(m.type) << ',' << int(m.rotationQuarterTurns) << ',' << int(m.decorationVariant)
                << ',' << int(m.connectionMask) << ',' << int(m.upperConnectionMask) << ',' << m.upperNeighbor
                << ',' << m.mainPath << ',' << m.branchId << ',' << int(m.inputDirection) << ',' << int(m.outputDirection);
            for (int neighbor : m.neighbors)
                result << ',' << neighbor;
        }
        return result.str();
    }
}

int main(int _argc, char** _argv)
{
    try
    {
        using namespace World;
        const int count = _argc > 1 ? std::stoi(_argv[1]) : 1000;
        int minRooms = 1000;
        int maxRooms = 0;
        int minDistance = 1000;
        int maxDistance = 0;
        int loops = 0;
        int fallbacks = 0;
        std::set<std::string> shapes;
        for (int seed = 0; seed < count; ++seed)
        {
            sForestDungeon dungeon;
            dungeon.bossId = sBossId::ForestSporecap;
            GenerateMushroomDungeonLayout(seed, dungeon);
            const auto original = dungeon.mushroomLayout;
            std::string error;
            Require(ValidateMushroomDungeonLayout(original, &error), "Seed " + std::to_string(seed) + ": " + error);
            GenerateMushroomDungeonLayout(seed, dungeon);
            Require(Fingerprint(original) == Fingerprint(dungeon.mushroomLayout), "Nondeterministic seed " + std::to_string(seed));
            minRooms = std::min(minRooms, int(original.modules.size()) - 1);
            maxRooms = std::max(maxRooms, int(original.modules.size()) - 1);
            minDistance = std::min(minDistance, original.bossDistance);
            maxDistance = std::max(maxDistance, original.bossDistance);
            loops += original.loopCount;
            fallbacks += original.generationAttempt == 32;
            std::ostringstream positions;
            for (const auto& m : original.modules)
                positions << m.floorIndex << ',' << m.cellX << ',' << m.cellZ << ';';
            shapes.insert(positions.str());
            if (seed == 42)
                std::cout << DescribeMushroomDungeonLayout(original);

            auto damaged = original;
            damaged.modules[0].neighbors[0] = -1;
            Require(!ValidateMushroomDungeonLayout(damaged, &error) && !error.empty(), "Accepted dangling entrance");
            damaged = original;
            const auto stair = std::find_if(damaged.modules.begin(), damaged.modules.end(),
                [](const auto& _rModule) { return _rModule.type == sMushroomModuleType::VerticalTransition; });
            stair->rotationQuarterTurns = (stair->rotationQuarterTurns + 1) % 4;
            Require(!ValidateMushroomDungeonLayout(damaged, &error), "Accepted misrotated stair");
            damaged = original;
            damaged.modules[2].cellX = damaged.modules[1].cellX;
            damaged.modules[2].cellZ = damaged.modules[1].cellZ;
            Require(!ValidateMushroomDungeonLayout(damaged, &error), "Accepted overlapping rooms");
        }
        // Exercise many fallback decorations plus the exact fixed seed used by the game.
        for (int seed = 0; seed <= count; ++seed)
        {
            std::mt19937 random(seed == count ? 0x46414c4cu : static_cast<uint32_t>(seed));
            sDungeonGraph graph;
            Require(GenerateMainPath(graph, random, true), "Fallback main path failed");
            GenerateBranches(graph, random, true);
            AssignRoomCategories(graph, random);
            auto layout = BuildMushroomDungeonLayout(graph, seed, 32, random);
            std::string error;
            Require(ValidateMushroomDungeonLayout(layout, &error), "Fallback seed " + std::to_string(seed) + ": " + error);
        }
        for (const int seed : { -1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() })
        {
            sForestDungeon dungeon;
            dungeon.bossId = sBossId::ForestSporecap;
            GenerateMushroomDungeonLayout(seed, dungeon);
            Require(ValidateMushroomDungeonLayout(dungeon.mushroomLayout), "Extreme seed failed");
        }
        for (int mask = 1; mask < 16; ++mask)
        {
            Require(RotateConnectionMask(mask, 4) == mask, "Mask full rotation failed");
            for (int rotation = 0; rotation < 4; ++rotation)
                Require(CanModuleSatisfyConnections(sMushroomModuleType::SmallCombat, mask, 0, rotation), "Generic socket failed");
        }
        Require(shapes.size() > size_t(count * .9), "Insufficient layout diversity");
        std::cout << count << " seeds + repeats + forced fallbacks passed. Rooms " << minRooms << "-" << maxRooms
            << ", boss distance " << minDistance << "-" << maxDistance << ", loops " << loops
            << ", natural fallbacks " << fallbacks << ", distinct placements " << shapes.size() << '\n';
        return 0;
    }
    catch (const std::exception& _rError)
    {
        std::cerr << _rError.what() << '\n';
        return 1;
    }
}
