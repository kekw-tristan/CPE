#include "collisionWorld.h"

#include "math/vector3.h"
#include "physics/collider.h"
#include "physics/collisionDetection.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Physics
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        constexpr float c_spatialCellSize = 8.0f;

        struct sCellCoordinate
        {
            int32_t x;
            int32_t z;

            bool operator==(const sCellCoordinate&) const = default;
        };

        struct sCellCoordinateHash
        {
            std::size_t operator()(const sCellCoordinate& _rCoordinate) const noexcept
            {
                const std::size_t xHash = std::hash<int32_t>{}(_rCoordinate.x);
                const std::size_t zHash = std::hash<int32_t>{}(_rCoordinate.z);
                return xHash ^ (zHash + 0x9e3779b9u + (xHash << 6) + (xHash >> 2));
            }
        };

        int32_t ToCell(float _position)
        {
            return static_cast<int32_t>(std::floor(_position / c_spatialCellSize));
        }

        class cCollisionWorld
        {

            public:

                static cCollisionWorld& GetInstance();

            public:

                sColliderHandle AddCollider(const sAABBCollider& _rCollider);
                void RemoveCollider(sColliderHandle _handle);
                void Clear();
                Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement);
                bool FindGroundHeight(const Math::cVec3f& _rPosition, float _maximumHeight, float& _rGroundHeight) const;

            private:

                cCollisionWorld();
                ~cCollisionWorld(); 

                cCollisionWorld(const cCollisionWorld&)              = delete;
                cCollisionWorld& operator=(const cCollisionWorld&)   = delete; 

                cCollisionWorld(const cCollisionWorld&&)             = delete;
                cCollisionWorld& operator=(const cCollisionWorld&&)  = delete;

            private:

                std::vector<sAABBCollider> m_colliders;
                std::vector<uint64_t>     m_generations;
                std::vector<std::size_t>  m_freeSlots;

                uint64_t m_nextGeneration = 1;

                std::unordered_map<sCellCoordinate, std::vector<std::size_t>, sCellCoordinateHash> m_spatialGrid;

                std::vector<std::size_t> FindCandidates(float _minX, float _maxX, float _minZ, float _maxZ) const;
        };
        
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cCollisionWorld& cCollisionWorld::GetInstance()
        {
            static cCollisionWorld s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sColliderHandle cCollisionWorld::AddCollider(const sAABBCollider& _rCollider)
        {
            const std::size_t colliderIndex = m_freeSlots.empty() ? m_colliders.size() : m_freeSlots.back();

            if (m_freeSlots.empty())
            {
                m_colliders.push_back(_rCollider);
                m_generations.push_back(0);
            }
            else
            {
                m_freeSlots.pop_back();
                m_colliders[colliderIndex] = _rCollider;
            }

            m_generations[colliderIndex] = m_nextGeneration++;

            const int32_t minX = ToCell(_rCollider.center.x() - _rCollider.halfExtents.x());
            const int32_t maxX = ToCell(_rCollider.center.x() + _rCollider.halfExtents.x());
            const int32_t minZ = ToCell(_rCollider.center.z() - _rCollider.halfExtents.z());
            const int32_t maxZ = ToCell(_rCollider.center.z() + _rCollider.halfExtents.z());

            for (int32_t z = minZ; z <= maxZ; ++z)
            {
                for (int32_t x = minX; x <= maxX; ++x)
                {
                    m_spatialGrid[{ x, z }].push_back(colliderIndex);
                }
            }

            return { colliderIndex, m_generations[colliderIndex] };
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cCollisionWorld::RemoveCollider(sColliderHandle _handle)
        {
            if (_handle.index >= m_generations.size() || _handle.generation == 0
                || m_generations[_handle.index] != _handle.generation)
                return;

            const auto& collider = m_colliders[_handle.index];

            const int32_t minX = ToCell(collider.center.x() - collider.halfExtents.x());
            const int32_t maxX = ToCell(collider.center.x() + collider.halfExtents.x());
            const int32_t minZ = ToCell(collider.center.z() - collider.halfExtents.z());
            const int32_t maxZ = ToCell(collider.center.z() + collider.halfExtents.z());

            for (int32_t z = minZ; z <= maxZ; ++z)
            {
                for (int32_t x = minX; x <= maxX; ++x)
                {
                    auto cell = m_spatialGrid.find({ x, z });
                    if (cell == m_spatialGrid.end())
                        continue;

                    std::erase(cell->second, _handle.index);
                    if (cell->second.empty())
                        m_spatialGrid.erase(cell);
                }
            }

            m_generations[_handle.index] = 0;
            m_freeSlots.push_back(_handle.index);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cCollisionWorld::Clear()
        {
            m_colliders.clear();
            m_generations.clear();
            m_freeSlots.clear();
            m_spatialGrid.clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f cCollisionWorld::MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement)
        {
            constexpr uint32_t c_maxIterations = 4;
            const float movementLength = _rMovement.length();
            const float maximumStepLength = std::max(_rCapsule.radius * 0.5f, 0.05f);
            const uint32_t stepCount = std::max(1u, static_cast<uint32_t>(std::ceil(movementLength / maximumStepLength)));
            const Math::cVec3f movementStep = _rMovement * (1.0f / static_cast<float>(stepCount));

            sCapsuleCollider movedCapsule = _rCapsule;

            for (uint32_t step = 0; step < stepCount; ++step)
            {
                movedCapsule.center += movementStep;

                for (uint32_t iteration = 0; iteration < c_maxIterations; ++iteration)
                {
                    bool collisionFound = false;
                    const std::vector<std::size_t> candidates = FindCandidates(
                        movedCapsule.center.x() - movedCapsule.radius,
                        movedCapsule.center.x() + movedCapsule.radius,
                        movedCapsule.center.z() - movedCapsule.radius,
                        movedCapsule.center.z() + movedCapsule.radius);

                    for (const std::size_t colliderIndex : candidates)
                    {
                        const sAABBCollider& collider = m_colliders[colliderIndex];
                        if (collider.isGround)
                            continue;

                        sCollisionResult result{};
                        if (!IntersectCapsuleAABB(movedCapsule, collider, result))
                            continue;

                        movedCapsule.center += result.normal * result.penetrationDepth;
                        collisionFound = true;
                    }

                    if (!collisionFound)
                        break;
                }
            }

            return movedCapsule.center;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool cCollisionWorld::FindGroundHeight(const Math::cVec3f& _rPosition, float _maximumHeight, float& _rGroundHeight) const
        {
            const std::vector<std::size_t> candidates = FindCandidates(_rPosition.x(), _rPosition.x(), _rPosition.z(), _rPosition.z());
            float bestHeight = -std::numeric_limits<float>::infinity();

            for (const std::size_t colliderIndex : candidates)
            {
                const sAABBCollider& collider = m_colliders[colliderIndex];
                if (!collider.isGround)
                    continue;

                const Math::cVec3f minimum = collider.center - collider.halfExtents;
                const Math::cVec3f maximum = collider.center + collider.halfExtents;
                if (_rPosition.x() < minimum.x() || _rPosition.x() > maximum.x() ||
                    _rPosition.z() < minimum.z() || _rPosition.z() > maximum.z() || maximum.y() > _maximumHeight)
                    continue;

                bestHeight = std::max(bestHeight, maximum.y());
            }

            if (!std::isfinite(bestHeight))
                return false;

            _rGroundHeight = bestHeight;
            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<std::size_t> cCollisionWorld::FindCandidates(float _minX, float _maxX, float _minZ, float _maxZ) const
        {
            std::unordered_set<std::size_t> uniqueCandidates;

            for (int32_t z = ToCell(_minZ); z <= ToCell(_maxZ); ++z)
            {
                for (int32_t x = ToCell(_minX); x <= ToCell(_maxX); ++x)
                {
                    const auto cell = m_spatialGrid.find({ x, z });
                    if (cell != m_spatialGrid.end())
                        uniqueCandidates.insert(cell->second.begin(), cell->second.end());
                }
            }

            return { uniqueCandidates.begin(), uniqueCandidates.end() };
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cCollisionWorld::cCollisionWorld()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cCollisionWorld::~cCollisionWorld()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace CollisionWorld
    {

        // -------------------------------------------------------------------------------------------------------------------------

        sColliderHandle AddCollider(const sAABBCollider& _rCollider)
        {
            return cCollisionWorld::GetInstance().AddCollider(_rCollider);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void RemoveCollider(sColliderHandle _handle)
        {
            cCollisionWorld::GetInstance().RemoveCollider(_handle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void Clear()
        {
            cCollisionWorld::GetInstance().Clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement)
        {
            return cCollisionWorld::GetInstance().MoveCapsule(_rCapsule, _rMovement);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool FindGroundHeight(const Math::cVec3f& _rPosition, float _maximumHeight, float& _rGroundHeight)
        {
            return cCollisionWorld::GetInstance().FindGroundHeight(_rPosition, _maximumHeight, _rGroundHeight);
        }

        // -------------------------------------------------------------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------------------------------------------------------
}


