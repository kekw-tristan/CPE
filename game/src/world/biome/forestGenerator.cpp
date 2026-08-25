#include "forestGenerator.h"

#include "../chunk.h"
#include "../worldConfig.h"
#include "../worldModels.h"

#include "graphics/scene/scene.h"

#include <random>

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
            groundInstance.transform.scale    = Math::cVec3f(1.0f, 1.0f, 1.0f);

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

        void GenerateTrees(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator, const sWorldLayout& _rWorldLayout)
        {
            constexpr uint32_t c_minTreeCount         = 20;
            constexpr uint32_t c_maxTreeCount         = 40;
            constexpr uint32_t c_maxPlacementAttempts = 500;

            constexpr float c_treeBorder        = 0.5f;
            constexpr float c_treeMinDistance   = 2.5f;
            constexpr float c_pathClearance     = 4.0f;
            constexpr float c_minScale          = 0.85f;
            constexpr float c_maxScale          = 1.15f;
            constexpr float c_twoPi             = 6.28318530718f;

            const float worldX = static_cast<float>(_rChunk.coordinate.x * c_chunkSize);
            const float worldY = _rChunk.height + 1.0f;
            const float worldZ = static_cast<float>(_rChunk.coordinate.z * c_chunkSize);

            const float halfChunkSize = static_cast<float>(c_chunkSize) * 0.5f;
            const float minOffset     = -halfChunkSize + c_treeBorder;
            const float maxOffset     = halfChunkSize - c_treeBorder;

            std::uniform_int_distribution<uint32_t> treeCountDistribution(c_minTreeCount, c_maxTreeCount);
            std::uniform_real_distribution<float>   positionDistribution(minOffset, maxOffset);
            std::uniform_real_distribution<float>   rotationDistribution(0.0f, c_twoPi);
            std::uniform_real_distribution<float>   scaleDistribution(c_minScale, c_maxScale);
            std::uniform_int_distribution<uint32_t> treeModelDistribution(0, 1);

            const uint32_t targetTreeCount = treeCountDistribution(_rRandomGenerator);

            std::vector<Math::cVec3f> treePositions;
            treePositions.reserve(targetTreeCount);

            uint32_t attempts = 0;

            while (treePositions.size() < targetTreeCount && attempts < c_maxPlacementAttempts)
            {
                ++attempts;

                const float treeX = worldX + positionDistribution(_rRandomGenerator);
                const float treeZ = worldZ + positionDistribution(_rRandomGenerator);

                const Math::cVec3f treePosition(treeX, worldY, treeZ);

                if (DistanceToPath(treePosition, _rWorldLayout) < c_pathClearance)
                    continue;

                if (!IsTreePositionValid(treePosition, treePositions, c_treeMinDistance))
                    continue;

                const float treeRotation = rotationDistribution(_rRandomGenerator);
                const float treeScale    = scaleDistribution(_rRandomGenerator);

                GFX::sShapeInstance treeInstance{};

                treeInstance.modelHandle = treeModelDistribution(_rRandomGenerator) == 0 ? WorldModels::Get("tree") : WorldModels::Get("tree_02");

                treeInstance.transform.position = treePosition;
                treeInstance.transform.rotation = Math::cVec3f(0.0f, treeRotation, 0.0f);
                treeInstance.transform.scale    = Math::cVec3f(treeScale, treeScale, treeScale);

                _rScene.AddShapeInstance(treeInstance);

                treePositions.push_back(treePosition);
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------


    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ForestGenerator
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void GenerateChunk(GFX::cScene& _rScene, const sChunk& _rChunk, std::mt19937& _rRandomGenerator, sWorldLayout& _rWorldLayout)
        {
            GenerateGround(_rScene, _rChunk);
            GenerateTrees(_rScene, _rChunk, _rRandomGenerator, _rWorldLayout);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------