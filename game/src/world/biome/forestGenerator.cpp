#include "forestGenerator.h"

#include "../chunk.h"
#include "../worldConfig.h"
#include "../terrainHeight.h"
#include "../worldModels.h"
#include "../prefab.h"

#include "../enemy/enemySpawn.h"

#include "graphics/shapeModel/assetManager.h"
#include "graphics/scene/scene.h"
#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelManager.h"

#include "physics/collider.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <random>

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{

    using namespace Engine;

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        constexpr float c_forestSpawnCenterZ = 34.0f;
        constexpr float c_forestSpawnClearHalfExtent = 90.0f;

        // -------------------------------------------------------------------------------------------------------------------------

        bool IsInsideForestSpawnClearance(const Math::cVec3f& _rPosition, float _padding = 0.0f)
        {
            return std::abs(_rPosition.x()) <= c_forestSpawnClearHalfExtent + _padding
                && std::abs(_rPosition.z() - c_forestSpawnCenterZ) <= c_forestSpawnClearHalfExtent + _padding;
        }

        // -------------------------------------------------------------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateGround(GFX::cScene& _rScene, const sChunk& _rChunk, std::vector<Physics::sAABBCollider>& _rColliders)
        {
            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            GFX::sShapeInstance groundInstance{};

            groundInstance.modelHandle = WorldModels::Get("ground");

            groundInstance.transform.position   = Math::cVec3f(worldX, worldY, worldZ);
            groundInstance.transform.rotation   = Math::cVec3f(0.0f, 0.0f, 0.0f);
            groundInstance.transform.scale      = Math::cVec3f(1.0f, 1.0f, 1.0f);

            _rScene.AddShapeInstance(groundInstance);

            Physics::sAABBCollider groundCollider{};
            groundCollider.center       = Math::cVec3f(worldX, worldY - 0.1f, worldZ);
            groundCollider.halfExtents  = Math::cVec3f(static_cast<float>(c_chunkSize) * 0.5f, 0.1f, static_cast<float>(c_chunkSize) * 0.5f);
            groundCollider.isGround     = true;
            groundCollider.groundHeightSampler = GetTerrainSurfaceHeight;
            _rColliders.push_back(groundCollider);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float DistanceToPathSegment(const Math::cVec3f& _rPoint, const Math::cVec3f& _rStart, const Math::cVec3f& _rEnd)
        {
            const float segmentX = _rEnd.x() - _rStart.x();
            const float segmentZ = _rEnd.z() - _rStart.z();

            const float pointX = _rPoint.x() - _rStart.x();
            const float pointZ = _rPoint.z() - _rStart.z();

            const float segmentLengthSquared = segmentX * segmentX + segmentZ * segmentZ;

            if (segmentLengthSquared <= 0.0001f)
                return std::sqrt(pointX * pointX + pointZ * pointZ);

            const float t = std::clamp((pointX * segmentX + pointZ * segmentZ) / segmentLengthSquared, 0.0f, 1.0f);

            const float closestX = _rStart.x() + segmentX * t;
            const float closestZ = _rStart.z() + segmentZ * t;

            const float deltaX = _rPoint.x() - closestX;
            const float deltaZ = _rPoint.z() - closestZ;

            return std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float DistanceToPath(const Math::cVec3f& _rPosition, const sWorldLayout& _rWorldLayout)
        {
            if (_rPosition.x() * _rPosition.x() + _rPosition.z() * _rPosition.z() > (c_forestRadius - 6.0f) * (c_forestRadius - 6.0f))
                return 0.0f;
            if (_rPosition.x() * _rPosition.x() + _rPosition.z() * _rPosition.z() < 16.0f * 16.0f)
                return 0.0f;
            // Reserve the reference temple and its front staircase.
            if (std::abs(_rPosition.x()) < 23.0f && _rPosition.z() > -3.0f && _rPosition.z() < 57.0f)
                return 0.0f;
            for (const auto& dungeon : _rWorldLayout.dungeons)
            {
                if (std::abs(_rPosition.x() - dungeon.center.x()) < 20.0f
                    && _rPosition.z() - dungeon.center.z() > -32.0f
                    && _rPosition.z() - dungeon.center.z() < 20.0f)
                    return 0.0f;
            }
            if (_rWorldLayout.mainPath.size() < 2)
                return std::numeric_limits<float>::max();

            float minDistance = std::numeric_limits<float>::max();

            for (size_t i = 0; i + 1 < _rWorldLayout.mainPath.size(); ++i)
            {
                const float distance = DistanceToPathSegment(_rPosition, _rWorldLayout.mainPath[i].position, _rWorldLayout.mainPath[i + 1].position);
                minDistance = std::min(minDistance, distance);
            }

            return minDistance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        struct sForestClearing
        {
            Math::cVec3f center;
            float radius = 0.0f;
            uint32_t variant = 0;
            uint32_t landmarkVariant = 0;
            bool landmark = false;
        };

        // -------------------------------------------------------------------------------------------------------------------------

        float GetSlope(float _x, float _z)
        {
            const float dx = (GetTerrainHeight(_x + 1.0f, _z) - GetTerrainHeight(_x - 1.0f, _z)) * 0.5f;
            const float dz = (GetTerrainHeight(_x, _z + 1.0f) - GetTerrainHeight(_x, _z - 1.0f)) * 0.5f;
            return std::sqrt(dx * dx + dz * dz);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool IsInsideClearing(const Math::cVec3f& _rPosition, const sForestClearing& _rClearing, float _padding = 0.0f)
        {
            const float dx = _rPosition.x() - _rClearing.center.x();
            const float dz = _rPosition.z() - _rClearing.center.z();
            const float radius = _rClearing.radius + _padding;
            return _rClearing.radius > 0.0f && dx * dx + dz * dz < radius * radius;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sForestClearing ChooseClearing(const sChunk& _rChunk, std::mt19937& _rRandomGenerator, const sWorldLayout& _rLayout)
        {
            sForestClearing clearing{};
            std::uniform_int_distribution<uint32_t> chance(0, 4);
            std::uniform_real_distribution<float> offset(-3.0f, 3.0f);
            std::uniform_int_distribution<uint32_t> variant(0, 7);
            if (chance(_rRandomGenerator) != 0)
                return clearing;

            const float x = static_cast<float>(_rChunk.coordinate.x * c_chunkSize) + offset(_rRandomGenerator);
            const float z = static_cast<float>(_rChunk.coordinate.z * c_chunkSize) + offset(_rRandomGenerator);
            clearing.center = { x, _rChunk.height + GetTerrainSurfaceHeight(x, z), z };
            if (IsInsideForestSpawnClearance(clearing.center, 24.0f)
                || DistanceToPath(clearing.center, _rLayout) < 26.0f
                || GetSlope(x, z) > 0.48f)
                return clearing;

            const uint32_t choice = variant(_rRandomGenerator);
            clearing.variant = choice % 3;
            clearing.landmarkVariant = choice % 4;
            clearing.landmark = choice >= 4
                && !IsInsideForestSpawnClearance(clearing.center, 46.0f)
                && DistanceToPath(clearing.center, _rLayout) >= 48.0f;
            if (clearing.landmark)
            {
                // Large footprints stay inside their owning chunk even after cardinal rotation.
                clearing.center = { static_cast<float>(_rChunk.coordinate.x * c_chunkSize),
                    _rChunk.height, static_cast<float>(_rChunk.coordinate.z * c_chunkSize) };
                clearing.center = Math::cVec3f(clearing.center.x(),
                    _rChunk.height + GetTerrainSurfaceHeight(clearing.center.x(), clearing.center.z()), clearing.center.z());
            }
            clearing.radius = clearing.landmark ? 43.0f : 18.0f + static_cast<float>(clearing.variant);
            return clearing;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool IsEnemyPositionFree(const Math::cVec3f& _rPosition, const std::vector<Physics::sAABBCollider>& _rColliders)
        {
            if (GetSlope(_rPosition.x(), _rPosition.z()) > 0.75f)
                return false;

            for (const auto& collider : _rColliders)
            {
                if (!collider.isGround
                    && std::abs(_rPosition.x() - collider.center.x()) < collider.halfExtents.x() + 1.5f
                    && std::abs(_rPosition.z() - collider.center.z()) < collider.halfExtents.z() + 1.5f)
                    return false;
            }
            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool IsTreePositionValid(const Math::cVec3f& _rPosition, const std::vector<Math::cVec3f>& _rTreePositions, float _minDistance)
        {
            const float minDistanceSquared = _minDistance * _minDistance;

            for (const Math::cVec3f& treePosition : _rTreePositions)
            {
                const float deltaX = _rPosition.x() - treePosition.x();
                const float deltaZ = _rPosition.z() - treePosition.z();
                const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;

                if (distanceSquared < minDistanceSquared)
                    return false;
            }

            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void AddAABBCollider(
            std::vector<Physics::sAABBCollider>& _rColliders,
            const Math::cVec3f& _rPosition,
            const Math::cVec3f& _rCenterOffset,
            const Math::cVec3f& _rHalfExtents,
            float _scale = 1.0f
        )
        {
            Physics::sAABBCollider collider{};

            collider.center = _rPosition + _rCenterOffset * _scale;
            collider.halfExtents = _rHalfExtents * _scale;

            _rColliders.push_back(collider);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateForestSpawn(
            GFX::cScene& _rScene,
            const sChunk& _rChunk
        )
        {
            if (_rChunk.coordinate.x != 0 || _rChunk.coordinate.z != 0)
                return;


            static GFX::sAssetHandle s_forestSpawnAsset{};
            static bool s_forestSpawnAssetLoaded = false;

            if (!s_forestSpawnAssetLoaded)
            {
                try
                {
                    s_forestSpawnAsset = GFX::AssetManager::Load(
                        "./assets/prefabs/forest_spawn.prefab.json"
                    );

                    s_forestSpawnAssetLoaded = true;
                }
                catch (const std::exception& exception)
                {
                    std::cerr << "Failed to load forest spawn prefab: " << exception.what() << '\n';
                    return;
                }
            }


            if (!s_forestSpawnAsset.IsValid())
                return;


            if (s_forestSpawnAsset.type != GFX::sAssetType::Prefab)
                return;


            GFX::sTransform transform{};

            transform.position = Math::cVec3f(
                0.0f,
                _rChunk.height + GetTerrainSurfaceHeight(0.0f, c_forestSpawnCenterZ),
                c_forestSpawnCenterZ
            );

            transform.rotation = Math::cVec3f(
                0.0f,
                0.0f,
                0.0f
            );

            transform.scale = Math::cVec3f(
                1.0f,
                1.0f,
                1.0f
            );


            InstantiatePrefab(
                _rScene,
                static_cast<GFX::PrefabHandle>(s_forestSpawnAsset.handle),
                transform
            );
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateTrees(
            GFX::cScene& _rScene,
            const sChunk& _rChunk,
            std::mt19937& _rRandomGenerator,
            const sWorldLayout& _rWorldLayout,
            const sForestClearing& _rClearing,
            std::vector<Physics::sAABBCollider>& _rColliders
        )
        {
            constexpr uint32_t c_minTreeCount           = 10;
            constexpr uint32_t c_maxTreeCount           = 28;
            constexpr uint32_t c_minStoneCount          = 3;
            constexpr uint32_t c_maxStoneCount          = 8;
            constexpr uint32_t c_maxPlacementAttempts   = 500;

            constexpr float c_treeScaleMultiplier   = 3.0f;
            constexpr float c_minTreeScale          = 0.65f;
            constexpr float c_maxTreeScale          = 1.25f;
            constexpr float c_treeModelMaxRadius    = 1.5f;
            constexpr float c_treeMaxRadius         = c_treeModelMaxRadius * c_maxTreeScale * c_treeScaleMultiplier;
            constexpr float c_treeBorder            = c_treeMaxRadius;
            constexpr float c_treeMinDistance       = c_treeMaxRadius * 2.0f;
            constexpr float c_stoneMinDistance      = 1.5f;
            constexpr float c_pathClearance         = 4.0f;
            constexpr float c_minStoneScale         = 0.7f;
            constexpr float c_maxStoneScale         = 1.3f;
            constexpr float c_twoPi                 = 6.28318530718f;

            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            const float halfChunkSize = static_cast<float>(c_chunkSize) * 0.5f;
            const float minOffset = -halfChunkSize + c_treeBorder;
            const float maxOffset = halfChunkSize - c_treeBorder;

            std::uniform_int_distribution<uint32_t> treeCountDistribution(c_minTreeCount, c_maxTreeCount);
            std::uniform_int_distribution<uint32_t> stoneCountDistribution(c_minStoneCount, c_maxStoneCount);
            std::uniform_real_distribution<float>   positionDistribution(minOffset, maxOffset);
            std::uniform_real_distribution<float>   rotationDistribution(0.0f, c_twoPi);
            std::uniform_real_distribution<float>   treeScaleDistribution(c_minTreeScale, c_maxTreeScale);
            std::uniform_real_distribution<float>   stoneScaleDistribution(c_minStoneScale, c_maxStoneScale);
            std::uniform_int_distribution<uint32_t> treeModelDistribution(0, 1);
            std::uniform_int_distribution<uint32_t> stoneModelDistribution(0, 2);

            const float groveDensity = 0.55f + 0.45f * std::sin(worldX * 0.021f + std::cos(worldZ * 0.017f) * 2.0f);
            const uint32_t targetTreeCount = static_cast<uint32_t>(treeCountDistribution(_rRandomGenerator) * (0.45f + groveDensity));

            std::vector<Math::cVec3f> treePositions;
            treePositions.reserve(targetTreeCount);

            uint32_t attempts = 0;

            while (treePositions.size() < targetTreeCount && attempts < c_maxPlacementAttempts)
            {
                ++attempts;

                const float treeX = worldX + positionDistribution(_rRandomGenerator);
                const float treeZ = worldZ + positionDistribution(_rRandomGenerator);

                const Math::cVec3f treeCandidatePosition(
                    treeX,
                    worldY + GetTerrainSurfaceHeight(treeX, treeZ),
                    treeZ
                );

                // Keep the entire canopy outside the spawn clearing.
                if (IsInsideForestSpawnClearance(treeCandidatePosition, c_treeMaxRadius)
                    || IsInsideClearing(treeCandidatePosition, _rClearing, c_treeMaxRadius)
                    || GetSlope(treeX, treeZ) > 0.70f)
                    continue;

                if (DistanceToPath(treeCandidatePosition, _rWorldLayout) < c_pathClearance + c_treeMaxRadius)
                    continue;

                if (!IsTreePositionValid(treeCandidatePosition, treePositions, c_treeMinDistance))
                    continue;

                const float treeRotation = rotationDistribution(_rRandomGenerator);
                const float treeScale = treeScaleDistribution(_rRandomGenerator) * c_treeScaleMultiplier;

                const Math::cVec3f treePosition(
                    treeX,
                    treeCandidatePosition.y() + treeScale,
                    treeZ
                );

                GFX::sShapeInstance treeInstance{};

                treeInstance.modelHandle =
                    treeCandidatePosition.y() > 40.0f ? WorldModels::Get("mountain_fir")
                    : treeModelDistribution(_rRandomGenerator) == 0
                    ? WorldModels::Get("tree_01")
                    : WorldModels::Get("tree_02");

                treeInstance.transform.position = treePosition;
                treeInstance.transform.rotation = Math::cVec3f(0.0f, treeRotation, 0.0f);
                treeInstance.transform.scale = Math::cVec3f(treeScale, treeScale, treeScale);

                _rScene.AddShapeInstance(treeInstance);

                treePositions.push_back(treePosition);

                AddAABBCollider(
                    _rColliders,
                    treePosition,
                    Math::cVec3f(0.0f, 1.0f, 0.0f),
                    Math::cVec3f(0.5f, 1.5f, 0.5f),
                    treeScale
                );
            }

            const uint32_t targetStoneCount = stoneCountDistribution(_rRandomGenerator);

            std::vector<Math::cVec3f> stonePositions;
            stonePositions.reserve(targetStoneCount);

            attempts = 0;

            while (stonePositions.size() < targetStoneCount && attempts < c_maxPlacementAttempts)
            {
                ++attempts;

                const float stoneX = worldX + positionDistribution(_rRandomGenerator);
                const float stoneZ = worldZ + positionDistribution(_rRandomGenerator);

                const Math::cVec3f stonePosition(stoneX, worldY + GetTerrainSurfaceHeight(stoneX, stoneZ), stoneZ);

                if (IsInsideForestSpawnClearance(stonePosition, c_maxStoneScale * 1.5f)
                    || IsInsideClearing(stonePosition, _rClearing, 2.0f))
                    continue;

                if (DistanceToPath(stonePosition, _rWorldLayout) < c_pathClearance)
                    continue;

                if (!IsTreePositionValid(stonePosition, treePositions, c_treeMinDistance))
                    continue;

                if (!IsTreePositionValid(stonePosition, stonePositions, c_stoneMinDistance))
                    continue;

                const float stoneRotation = rotationDistribution(_rRandomGenerator);
                const float stoneScale = stoneScaleDistribution(_rRandomGenerator);

                GFX::sShapeInstance stoneInstance{};

                switch (stoneModelDistribution(_rRandomGenerator))
                {
                case 0:
                    stoneInstance.modelHandle = WorldModels::Get("stone_01");
                    break;

                case 1:
                    stoneInstance.modelHandle = WorldModels::Get("stone_02");
                    break;

                case 2:
                    stoneInstance.modelHandle = WorldModels::Get("stone_03");
                    break;
                }

                stoneInstance.transform.position = stonePosition;
                stoneInstance.transform.rotation = Math::cVec3f(0.0f, stoneRotation, 0.0f);
                stoneInstance.transform.scale = Math::cVec3f(stoneScale, stoneScale, stoneScale);

                _rScene.AddShapeInstance(stoneInstance);

                stonePositions.push_back(stonePosition);

                AddAABBCollider(
                    _rColliders,
                    stonePosition,
                    Math::cVec3f(0.0f, 0.5f, 0.0f),
                    Math::cVec3f(0.7f, 0.5f, 0.7f),
                    stoneScale
                );
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool GenerateLandmark(
            GFX::cScene& _rScene,
            const sChunk& _rChunk,
            const sForestClearing& _rClearing,
            std::vector<sEnemySpawn>& _rSpawns
        )
        {
            if (!_rClearing.landmark)
                return false;

            const uint32_t variant = _rClearing.landmarkVariant;
            constexpr int c_halfWidths[] = { 25, 27, 27, 25 };
            constexpr int c_halfDepths[] = { 27, 26, 25, 27 };
            constexpr float c_entranceZ[] = { -27.0f, -26.0f, -24.0f, -27.0f };
            constexpr float c_entranceHeight[] = { -0.12f, 0.0f, -0.12f, 0.0f };
            constexpr float c_stairStart = -31.0f;

            // Face the entrance toward the uphill approach, keeping the staircase short and walkable.
            float angle = 0.0f;
            float entryHeight = -std::numeric_limits<float>::max();
            for (int direction = 0; direction < 4; ++direction)
            {
                const float candidateAngle = static_cast<float>(direction) * 1.5707963f;
                const float height = GetTerrainSurfaceHeight(
                    _rClearing.center.x() - std::sin(candidateAngle) * 31.0f,
                    _rClearing.center.z() - std::cos(candidateAngle) * 31.0f);
                if (height > entryHeight)
                {
                    angle = candidateAngle;
                    entryHeight = height;
                }
            }

            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            const auto terrainAt = [&](float _x, float _z)
            {
                return GetTerrainSurfaceHeight(_rClearing.center.x() + _x * cosine + _z * sine,
                    _rClearing.center.z() - _x * sine + _z * cosine);
            };

            float floorHeight = entryHeight;
            float foundationHeight = entryHeight;
            for (int z = -c_halfDepths[variant]; z <= c_halfDepths[variant]; z += 2)
            {
                for (int x = -c_halfWidths[variant]; x <= c_halfWidths[variant]; x += 2)
                {
                    const float height = terrainAt(static_cast<float>(x), static_cast<float>(z));
                    floorHeight = std::max(floorHeight, height);
                    foundationHeight = std::min(foundationHeight, height);
                }
            }
            // Include the entire stair approach in the clearance calculation.
            for (int z = -31; z < static_cast<int>(c_entranceZ[variant]); ++z)
                floorHeight = std::max(floorHeight, terrainAt(0.0f, static_cast<float>(z)));
            floorHeight += 0.7f;
            const float stairRun = c_entranceZ[variant] - c_stairStart;
            if (floorHeight - entryHeight > stairRun * 0.8f || floorHeight - foundationHeight > 27.0f)
                return false;

            static const char* c_prefabPaths[] =
            {
                "./assets/prefabs/astral_observatory.prefab.json",
                "./assets/prefabs/hollow_cathedral.prefab.json",
                "./assets/prefabs/thorn_sanctum.prefab.json",
                "./assets/prefabs/moon_belfry.prefab.json"
            };
            static GFX::sAssetHandle s_assets[4]{};
            static bool s_attempted[4]{};
            if (!s_attempted[variant])
            {
                s_attempted[variant] = true;
                try
                {
                    s_assets[variant] = GFX::AssetManager::Load(c_prefabPaths[variant]);
                }
                catch (const std::exception& exception)
                {
                    std::cerr << "Failed to load dungeon landmark: " << exception.what() << '\n';
                }
            }
            if (!s_assets[variant].IsValid() || s_assets[variant].type != GFX::sAssetType::Prefab)
                return false;

            GFX::sTransform transform{};
            transform.position = { _rClearing.center.x(), _rChunk.height + floorHeight, _rClearing.center.z() };
            transform.rotation = { 0.0f, angle, 0.0f };
            transform.scale = { 1.0f, 1.0f, 1.0f };
            InstantiatePrefab(_rScene, static_cast<GFX::PrefabHandle>(s_assets[variant].handle), transform);

            // Treads meet at their edges; overlapping side faces would flicker.
            constexpr int c_stepCount = 32;
            const float treadLength = stairRun / c_stepCount;
            for (int step = 0; step < c_stepCount; ++step)
            {
                const float z = c_stairStart + (static_cast<float>(step) + 0.5f) * treadLength;
                const float top = entryHeight + (floorHeight + c_entranceHeight[variant] - entryHeight) * static_cast<float>(step + 1) / c_stepCount;
                const float depth = std::max(1.0f, top - std::min({ terrainAt(-3.5f, z), terrainAt(0.0f, z), terrainAt(3.5f, z) }) + 1.0f);
                GFX::sShapeInstance stair{};
                stair.modelHandle = WorldModels::Get("dungeon_step");
                stair.transform.position = { _rClearing.center.x() + z * sine, _rChunk.height + top,
                    _rClearing.center.z() + z * cosine };
                stair.transform.rotation = transform.rotation;
                stair.transform.scale = { 7.0f, depth, treadLength };
                stair.collisionMode = GFX::eShapeCollisionMode::Mesh;
                stair.generateLights = false;
                _rScene.AddShapeInstance(stair);
            }

            constexpr sEnemyType::Enum c_champions[] =
            {
                sEnemyType::ForestThornshooter, sEnemyType::ForestBarkguard, sEnemyType::ForestSporecap, sEnemyType::ForestRootcharger
            };
            // Undefined progression ID denotes a local miniboss, not one of the four forest guardians.
            _rSpawns.push_back({ c_champions[variant], transform.position, angle + 3.1415926f, true });
            for (int guard = 0; guard < 4; ++guard)
            {
                const float x = guard % 2 == 0 ? -6.0f : 6.0f;
                const float z = guard < 2 ? -7.0f : 5.0f;
                const Math::cVec3f position = transform.position + Math::cVec3f(x * cosine + z * sine, 0.0f, -x * sine + z * cosine);
                _rSpawns.push_back({ variant == 2 ? sEnemyType::ForestSporecap : sEnemyType::ForestRootcharger,
                    position, angle + 3.1415926f });
            }
            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateMountainDetails(
            GFX::cScene& _rScene,
            const sChunk& _rChunk,
            std::mt19937& _rRandomGenerator,
            const sWorldLayout& _rLayout,
            const sForestClearing& _rClearing,
            std::vector<Physics::sAABBCollider>& _rColliders,
            std::vector<sEnemySpawn>& _rSpawns
        )
        {
            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);
            std::uniform_real_distribution<float> offset(-25.0f, 25.0f);
            std::uniform_real_distribution<float> rotation(0.0f, 6.2831853f);
            std::uniform_real_distribution<float> scale(0.7f, 1.5f);

            const auto addDetail = [&](const char* _pModel, float _x, float _z, float _scale, float _rotation, bool _solid)
            {
                GFX::sShapeInstance instance{};
                instance.modelHandle = WorldModels::Get(_pModel);
                instance.transform.position = { _x, _rChunk.height + GetTerrainSurfaceHeight(_x, _z) - 0.15f, _z };
                instance.transform.rotation = { 0.0f, _rotation, 0.0f };
                instance.transform.scale = { _scale, _scale, _scale };
                // Small foliage and crystals need neither triangle colliders nor individual lights.
                instance.collisionMode = GFX::eShapeCollisionMode::Disabled;
                instance.generateLights = false;
                _rScene.AddShapeInstance(instance);
                if (_solid)
                {
                    AddAABBCollider(_rColliders, instance.transform.position,
                        { 0.0f, 1.5f, 0.0f }, { 1.25f, 1.5f, 1.25f }, _scale);
                }
            };

            // Clumps of low foliage alternate with exposed outcrops and luminous crystal seams.
            for (uint32_t i = 0; i < 30; ++i)
            {
                const float x = worldX + offset(_rRandomGenerator);
                const float z = worldZ + offset(_rRandomGenerator);
                const Math::cVec3f position(x, _rChunk.height + GetTerrainSurfaceHeight(x, z), z);
                if (IsInsideForestSpawnClearance(position, 5.0f)
                    || IsInsideClearing(position, _rClearing, 3.0f)
                    || DistanceToPath(position, _rLayout) < 7.0f
                    || !IsEnemyPositionFree(position, _rColliders))
                    continue;

                const float detailScale = scale(_rRandomGenerator);
                const float detailRotation = rotation(_rRandomGenerator);
                if (i < 4)
                {
                    addDetail("mountain_outcrop", x, z, detailScale * 1.7f, detailRotation, true);
                }
                else
                {
                    const bool crystal = i % 7 == 0;
                    const char* model = crystal ? "moon_crystals" : "moon_undergrowth";
                    addDetail(model, x, z, detailScale, detailRotation, false);
                }
            }

            if (_rClearing.radius <= 0.0f)
                return;

            if (GenerateLandmark(_rScene, _rChunk, _rClearing, _rSpawns))
                return;

            const float angle = rotation(_rRandomGenerator);
            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            std::vector<Physics::sAABBCollider> ruinSpawnClearances;
            ruinSpawnClearances.reserve(12);

            const auto addRuin = [&](const char* _pModel, float _x, float _z, float _rotation)
            {
                GFX::sShapeInstance instance{};
                instance.modelHandle = WorldModels::Get(_pModel);
                if (instance.modelHandle < 0)
                    return;

                const float x = _rClearing.center.x() + _x * cosine + _z * sine;
                const float z = _rClearing.center.z() - _x * sine + _z * cosine;
                const float yaw = angle + _rotation;
                const float localCosine = std::cos(yaw);
                const float localSine = std::sin(yaw);
                const auto& bounds = GFX::ShapeModelManager::GetShapeModel(instance.modelHandle).bounds;

                // Seat each masonry section independently; buried footings bridge the slope.
                float groundHeight = GetTerrainSurfaceHeight(x, z);
                for (int corner = 0; corner < 4; ++corner)
                {
                    const float localX = (corner & 1) ? bounds.max.x() : bounds.min.x();
                    const float localZ = (corner & 2) ? bounds.max.z() : bounds.min.z();
                    groundHeight = std::min(groundHeight, GetTerrainSurfaceHeight(
                        x + localX * localCosine + localZ * localSine,
                        z - localX * localSine + localZ * localCosine));
                }

                instance.transform.position = { x, _rChunk.height + groundHeight, z };
                instance.transform.rotation = { 0.0f, yaw, 0.0f };
                instance.transform.scale = { 1.0f, 1.0f, 1.0f };
                instance.generateLights = false;
                // Mesh collision preserves doorways and the spaces beneath the broken arches.
                instance.collisionMode = GFX::eShapeCollisionMode::Mesh;
                _rScene.AddShapeInstance(instance);

                // Conservative bounds reserve enemy spawn space only, never block the doorway.
                Physics::sAABBCollider clearance{};
                clearance.center = instance.transform.position + Math::cVec3f(
                    bounds.center.x() * localCosine + bounds.center.z() * localSine,
                    bounds.center.y(),
                    -bounds.center.x() * localSine + bounds.center.z() * localCosine);
                clearance.halfExtents = {
                    (bounds.size.x() * std::abs(localCosine) + bounds.size.z() * std::abs(localSine)) * 0.5f,
                    bounds.size.y() * 0.5f,
                    (bounds.size.x() * std::abs(localSine) + bounds.size.z() * std::abs(localCosine)) * 0.5f
                };
                ruinSpawnClearances.push_back(clearance);
            };

            constexpr float c_halfPi = 1.5707963f;
            constexpr float c_pi = 3.1415926f;

            // Roofless chapel, collapsed courtyard, and a crystal-overgrown gatehouse.
            // Broken side walls leave several routes into the central encounter space.
            addRuin(_rClearing.variant == 1 ? "ruin_broken_arch" : "ruin_arch", 0.0f, -10.0f, 0.0f);
            addRuin("ruin_corner", -7.0f, 9.0f, 0.0f);
            addRuin("ruin_corner", 7.0f, 9.0f, -c_halfPi);
            addRuin("ruin_wall", -9.0f, 2.0f, c_halfPi);
            addRuin("ruin_wall", 9.0f, -3.0f, -c_halfPi);
            addRuin("ruin_broken_arch", _rClearing.variant == 0 ? 0.0f : 8.0f,
                _rClearing.variant == 0 ? 9.0f : 4.0f, _rClearing.variant == 0 ? c_pi : -c_halfPi);
            addRuin("ruin_rubble", -10.0f, -6.0f, angle * 0.3f);
            addRuin("ruin_rubble", 5.0f, 11.0f, c_halfPi);

            if (_rClearing.variant == 1)
            {
                addRuin("ruin_wall", -5.0f, -10.0f, c_pi);
                addRuin("ruin_rubble", 10.0f, 8.0f, 0.0f);
            }
            else
            {
                addDetail("moon_crystals", _rClearing.center.x(), _rClearing.center.z(),
                    _rClearing.variant == 2 ? 2.0f : 1.3f, angle, true);
            }

            constexpr sEnemyType::Enum c_guardTypes[] =
            {
                sEnemyType::ForestBarkguard, sEnemyType::ForestRootcharger, sEnemyType::ForestSporecap
            };
            for (uint32_t i = 0; i < 3 + _rClearing.variant; ++i)
            {
                const float theta = angle + static_cast<float>(i) * 1.256637f;
                const float x = _rClearing.center.x() + std::cos(theta) * 6.0f;
                const float z = _rClearing.center.z() + std::sin(theta) * 6.0f;
                const Math::cVec3f position(x, _rChunk.height + GetTerrainSurfaceHeight(x, z), z);
                if (IsEnemyPositionFree(position, _rColliders)
                    && IsEnemyPositionFree(position, ruinSpawnClearances))
                {
                    _rSpawns.push_back({ c_guardTypes[_rClearing.variant], position, theta });
                }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateEnemyPacks(
            const sChunk& _rChunk,
            std::mt19937& _rRandomGenerator,
            const sWorldLayout& _rWorldLayout,
            const sForestClearing& _rClearing,
            const std::vector<Physics::sAABBCollider>& _rColliders,
            std::vector<sEnemySpawn>& _rEnemySpawns
        )
        {
            constexpr uint32_t c_minPackCount = 0;
            constexpr uint32_t c_maxPackCount = 2;

            constexpr uint32_t c_minEnemiesPerPack = 1;
            constexpr uint32_t c_maxEnemiesPerPack = 4;

            constexpr float c_packBorder = 5.0f;
            constexpr float c_packRadius = 3.0f;
            constexpr float c_pathClearance = 5.0f;
            constexpr float c_twoPi = 6.28318530718f;

            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            const float halfChunkSize = static_cast<float>(c_chunkSize) * 0.5f;

            std::uniform_int_distribution<uint32_t> packCountDistribution(c_minPackCount, c_maxPackCount);
            std::uniform_int_distribution<uint32_t> enemyCountDistribution(c_minEnemiesPerPack, c_maxEnemiesPerPack);

            std::uniform_real_distribution<float> packPositionDistribution(-halfChunkSize + c_packBorder, halfChunkSize - c_packBorder);
            std::uniform_real_distribution<float> packOffsetDistribution(-c_packRadius, c_packRadius);
            std::uniform_real_distribution<float> rotationDistribution(0.0f, c_twoPi);
            constexpr sEnemyType::Enum c_forestEnemyTypes[] =
            {
                sEnemyType::ForestSporecap,
                sEnemyType::ForestThornshooter,
                sEnemyType::ForestRootcharger,
                sEnemyType::ForestBarkguard
            };
            std::uniform_int_distribution<uint32_t> enemyTypeDistribution(0,
                static_cast<uint32_t>(sizeof(c_forestEnemyTypes) / sizeof(c_forestEnemyTypes[0])) - 1);

            const uint32_t packCount = packCountDistribution(_rRandomGenerator);

            for (uint32_t packIndex = 0; packIndex < packCount; ++packIndex)
            {
                const Math::cVec3f packCenter(worldX + packPositionDistribution(_rRandomGenerator), worldY, worldZ + packPositionDistribution(_rRandomGenerator));

                if (DistanceToPath(packCenter, _rWorldLayout) < c_pathClearance)
                    continue;

                const uint32_t enemyCount = enemyCountDistribution(_rRandomGenerator);

                for (uint32_t enemyIndex = 0; enemyIndex < enemyCount; ++enemyIndex)
                {
                    sEnemySpawn spawn{};

                    spawn.type = c_forestEnemyTypes[enemyTypeDistribution(_rRandomGenerator)];
                    spawn.position = Math::cVec3f(packCenter.x() + packOffsetDistribution(_rRandomGenerator), worldY, packCenter.z() + packOffsetDistribution(_rRandomGenerator));
                    spawn.position = Math::cVec3f(
                        spawn.position.x(),
                        worldY + GetTerrainSurfaceHeight(spawn.position.x(), spawn.position.z()),
                        spawn.position.z());
                    spawn.rotation = rotationDistribution(_rRandomGenerator);

                    if (!IsInsideForestSpawnClearance(spawn.position)
                        && !IsInsideClearing(spawn.position, _rClearing, 2.0f)
                        && IsEnemyPositionFree(spawn.position, _rColliders)
                        && DistanceToPath(spawn.position, _rWorldLayout) >= c_pathClearance)
                        _rEnemySpawns.push_back(spawn);

                }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------


    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ForestGenerator
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateDungeons(
            GFX::cScene& _rScene,
            const sWorldLayout& _rLayout,
            std::vector<sEnemySpawn>& _rSpawns,
            const sChunk& _rChunk,
            std::vector<Physics::sAABBCollider>& _rColliders
        )
        {
            const auto belongsToChunk = [&](const Math::cVec3f& _rPosition)
            {
                return static_cast<int>(std::floor(_rPosition.x() / c_chunkSize + 0.5f)) == _rChunk.coordinate.x
                    && static_cast<int>(std::floor(_rPosition.z() / c_chunkSize + 0.5f)) == _rChunk.coordinate.z;
            };

            const auto addSpawn = [&](const sEnemySpawn& _rSpawn)
            {
                if (belongsToChunk(_rSpawn.position) && !IsInsideForestSpawnClearance(_rSpawn.position))
                    _rSpawns.push_back(_rSpawn);
            };

            const auto addWall = [&](const Math::cVec3f& _rPosition, float _scale)
            {
                if (!belongsToChunk(_rPosition))
                    return;

                const Math::cVec3f wallPosition(
                    _rPosition.x(),
                    _rPosition.y() + GetTerrainSurfaceHeight(_rPosition.x(), _rPosition.z()),
                    _rPosition.z());

                GFX::sShapeInstance wall{};
                wall.modelHandle        = WorldModels::Get("stone_01");
                wall.transform.position = wallPosition;
                wall.transform.scale    = Math::cVec3f(_scale, 6.0f, _scale);

                _rScene.AddShapeInstance(wall);
                AddAABBCollider(_rColliders, wallPosition, Math::cVec3f(0.0f, 3.0f, 0.0f), Math::cVec3f(_scale, 3.0f, _scale));
            };


            // Small, non-blocking stones make the cleared routes readable on the grass.
            for (size_t i = 0; i + 1 < _rLayout.mainPath.size(); ++i)
            {
                const auto start    = _rLayout.mainPath[i].position;
                const auto delta    = _rLayout.mainPath[i + 1].position - start;
                const float length  = std::sqrt(delta.x() * delta.x() + delta.z() * delta.z());

                for (float distance = 1.0f; distance < length; distance += 3.0f)
                {
                    GFX::sShapeInstance marker{};
                    marker.modelHandle          = WorldModels::Get("stone_02");
                    marker.transform.position   = start + delta * (distance / length);
                    marker.transform.position = Math::cVec3f(marker.transform.position.x(),
                        GetTerrainSurfaceHeight(marker.transform.position.x(), marker.transform.position.z()) + 0.04f,
                        marker.transform.position.z());
                    marker.transform.scale      = Math::cVec3f(0.35f, 0.08f, 0.35f);

                    if (belongsToChunk(marker.transform.position)
                        && !IsInsideForestSpawnClearance(marker.transform.position, 0.5f))
                        _rScene.AddShapeInstance(marker);
                }
            }

            // Continuous cliff ring closes the currently playable forest section.
            for (int i = 0; i < c_forestWallCount; ++i)
            {
                const float angle = static_cast<float>(i) * 6.2831853f / static_cast<float>(c_forestWallCount);
                addWall(Math::cVec3f(std::cos(angle) * c_forestRadius, 0.0f, std::sin(angle) * c_forestRadius), 2.5f);
            }

            for (const auto& dungeon : _rLayout.dungeons)
            {
                // Roofless ruins: boss chamber, a southern doorway and a guarded approach.
                for (int offset = -14; offset <= 14; offset += 2)
                {
                    const float value = static_cast<float>(offset);

                    addWall(dungeon.center + Math::cVec3f(-14.0f, 0.f, value), 1.25f);
                    addWall(dungeon.center + Math::cVec3f(14.0f, 0.f, value), 1.25f);
                    addWall(dungeon.center + Math::cVec3f(value, 0.f, 14.0f), 1.25f);

                    if (std::abs(offset) >= 6)
                        addWall(dungeon.center + Math::cVec3f(value, 0.0f, -14.0f), 1.25f);
                }

                for (int offset = -26; offset < -14; offset += 2)
                {
                    addWall(dungeon.center + Math::cVec3f(-7.0f, 0.0f, static_cast<float>(offset)), 1.25f);
                    addWall(dungeon.center + Math::cVec3f(7.0f, 0.0f, static_cast<float>(offset)), 1.25f);
                }

                const Math::cVec3f bossPosition = dungeon.center;
                const Math::cVec3f leftPosition = dungeon.center + Math::cVec3f(-3.0f, 0.0f, -22.0f);
                const Math::cVec3f rightPosition = dungeon.center + Math::cVec3f(3.0f, 0.0f, -22.0f);

                addSpawn({ dungeon.type, Math::cVec3f(bossPosition.x(), GetTerrainSurfaceHeight(bossPosition.x(), bossPosition.z()), bossPosition.z()), 3.1415926f, true, dungeon.bossId });
                addSpawn({ dungeon.type, Math::cVec3f(leftPosition.x(), GetTerrainSurfaceHeight(leftPosition.x(), leftPosition.z()), leftPosition.z()), 3.1415926f });
                addSpawn({ dungeon.type, Math::cVec3f(rightPosition.x(), GetTerrainSurfaceHeight(rightPosition.x(), rightPosition.z()), rightPosition.z()), 3.1415926f });
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateChunk(
            GFX::cScene& _rScene,
            const sChunk& _rChunk,
            std::mt19937& _rRandomGenerator,
            sWorldLayout& _rWorldLayout,
            std::vector<sEnemySpawn>& _rEnemySpawns,
            std::vector<Physics::sAABBCollider>& _rColliders
        )
        {
            GenerateGround(_rScene, _rChunk, _rColliders);
            GenerateForestSpawn(_rScene, _rChunk);
            const sForestClearing clearing = ChooseClearing(_rChunk, _rRandomGenerator, _rWorldLayout);
            GenerateTrees(_rScene, _rChunk, _rRandomGenerator, _rWorldLayout, clearing, _rColliders);
            GenerateMountainDetails(_rScene, _rChunk, _rRandomGenerator, _rWorldLayout, clearing, _rColliders, _rEnemySpawns);
            GenerateEnemyPacks(_rChunk, _rRandomGenerator, _rWorldLayout, clearing, _rColliders, _rEnemySpawns);
            GenerateDungeons(_rScene, _rWorldLayout, _rEnemySpawns, _rChunk, _rColliders);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
