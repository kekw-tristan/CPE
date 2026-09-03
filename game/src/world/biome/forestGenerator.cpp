#include "forestGenerator.h"

#include "../chunk.h"
#include "../worldConfig.h"
#include "../worldModels.h"

#include "../enemy/enemySpawn.h"

#include "graphics/scene/scene.h"

#include "physics/collider.h"
#include "physics/collisionWorld.h"

#include <algorithm>

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{

    using namespace Engine;

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateGround(GFX::cScene& _rScene, const sChunk& _rChunk)
        {
            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            GFX::sShapeInstance groundInstance{};

            groundInstance.modelHandle = WorldModels::Get("ground");

            groundInstance.transform.position = Math::cVec3f(worldX, worldY, worldZ);
            groundInstance.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
            groundInstance.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

            _rScene.AddShapeInstance(groundInstance);

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

        void AddAABBCollider(const Math::cVec3f& _rPosition, const Math::cVec3f& _rCenterOffset, const Math::cVec3f& _rHalfExtents, float _scale = 1.0f)
        {
            Physics::sAABBCollider collider{};

            collider.center = _rPosition + _rCenterOffset * _scale;
            collider.halfExtents = _rHalfExtents * _scale;

            Physics::CollisionWorld::AddCollider(collider);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateTrees(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator, const sWorldLayout& _rWorldLayout)
        {
            constexpr uint32_t c_minTreeCount           = 20;
            constexpr uint32_t c_maxTreeCount           = 40;
            constexpr uint32_t c_minStoneCount          = 3;
            constexpr uint32_t c_maxStoneCount          = 8;
            constexpr uint32_t c_maxPlacementAttempts   = 500;

            constexpr float c_treeBorder        = 0.5f;
            constexpr float c_treeMinDistance   = 2.5f;
            constexpr float c_stoneMinDistance  = 1.5f;
            constexpr float c_pathClearance     = 4.0f;
            constexpr float c_minTreeScale      = 0.85f;
            constexpr float c_maxTreeScale      = 1.15f;
            constexpr float c_minStoneScale     = 0.7f;
            constexpr float c_maxStoneScale     = 1.3f;
            constexpr float c_twoPi             = 6.28318530718f;

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

            const uint32_t targetTreeCount = treeCountDistribution(_rRandomGenerator);

            std::vector<Math::cVec3f> treePositions;
            treePositions.reserve(targetTreeCount);

            uint32_t attempts = 0;

            while (treePositions.size() < targetTreeCount && attempts < c_maxPlacementAttempts)
            {
                ++attempts;

                const float treeX = worldX + positionDistribution(_rRandomGenerator);
                const float treeZ = worldZ + positionDistribution(_rRandomGenerator);

                const Math::cVec3f treePosition(treeX, worldY + 1.0f, treeZ);

                if (DistanceToPath(treePosition, _rWorldLayout) < c_pathClearance)
                    continue;

                if (!IsTreePositionValid(treePosition, treePositions, c_treeMinDistance))
                    continue;

                const float treeRotation = rotationDistribution(_rRandomGenerator);
                const float treeScale    = treeScaleDistribution(_rRandomGenerator);

                GFX::sShapeInstance treeInstance{};

                treeInstance.modelHandle = treeModelDistribution(_rRandomGenerator) == 0 ? WorldModels::Get("tree_01") : WorldModels::Get("tree_02");

                treeInstance.transform.position = treePosition;
                treeInstance.transform.rotation = Math::cVec3f(0.0f, treeRotation, 0.0f);
                treeInstance.transform.scale    = Math::cVec3f(treeScale, treeScale, treeScale);

                _rScene.AddShapeInstance(treeInstance);

                treePositions.push_back(treePosition);

                AddAABBCollider(
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

                const Math::cVec3f stonePosition(stoneX, worldY, stoneZ);

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
                    stonePosition,
                    Math::cVec3f(0.0f, 0.5f, 0.0f),
                    Math::cVec3f(0.7f, 0.5f, 0.7f),
                    stoneScale
                );
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateEnemyPacks(const sChunk& _rChunk, std::mt19937& _rRandomGenerator, const sWorldLayout& _rWorldLayout, std::vector<sEnemySpawn>& _rEnemySpawns)
        {
            constexpr uint32_t c_minPackCount = 0;
            constexpr uint32_t c_maxPackCount = 2;

            constexpr uint32_t c_minEnemiesPerPack = 3;
            constexpr uint32_t c_maxEnemiesPerPack = 6;

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
            std::uniform_int_distribution<uint32_t> enemyTypeDistribution(0, 1);

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

                    spawn.type = enemyTypeDistribution(_rRandomGenerator) == 0 ? sEnemyType::ForestCrawler : sEnemyType::ForestBrute;
                    spawn.position = Math::cVec3f(packCenter.x() + packOffsetDistribution(_rRandomGenerator), worldY, packCenter.z() + packOffsetDistribution(_rRandomGenerator));
                    spawn.rotation = rotationDistribution(_rRandomGenerator);

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

        void GenerateChunk(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator, sWorldLayout& _rWorldLayout, std::vector<sEnemySpawn>& _rEnemySpawns)
        {
            GenerateGround(_rScene, _rChunk);
            GenerateTrees(_rScene, _rChunk, _rRandomGenerator, _rWorldLayout);
            GenerateEnemyPacks(_rChunk, _rRandomGenerator, _rWorldLayout, _rEnemySpawns);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------