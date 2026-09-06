#include "collisionWorld.h"

#include "math/vector3.h"
#include "physics/collider.h"
#include "physics/collisionDetection.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
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
                sColliderHandle AddCollider(const sTriangleCollider& _rCollider);
                void RemoveCollider(sColliderHandle _handle);
                void Clear();
                Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement, float _maximumStepHeight);
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
                std::vector<std::optional<sTriangleCollider>> m_triangles;
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
                m_triangles.emplace_back();
                m_generations.push_back(0);
            }
            else
            {
                m_freeSlots.pop_back();
                m_colliders[colliderIndex] = _rCollider;
                m_triangles[colliderIndex].reset();
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

        sColliderHandle cCollisionWorld::AddCollider(const sTriangleCollider& _rCollider)
        {
            const Math::cVec3f vertices[] = { _rCollider.a, _rCollider.b, _rCollider.c };
            for (const auto& vertex : vertices)
            {
                if (!std::isfinite(vertex.x()) || !std::isfinite(vertex.y()) || !std::isfinite(vertex.z()))
                {
                    throw std::invalid_argument("Non-finite triangle collider");
                }
            }
            if ((_rCollider.b - _rCollider.a).cross(_rCollider.c - _rCollider.a).lengthSquared() <= 0.0f)
            {
                throw std::invalid_argument("Degenerate triangle collider");
            }

            const Math::cVec3f minimum{
                std::min({ _rCollider.a.x(), _rCollider.b.x(), _rCollider.c.x() }),
                std::min({ _rCollider.a.y(), _rCollider.b.y(), _rCollider.c.y() }),
                std::min({ _rCollider.a.z(), _rCollider.b.z(), _rCollider.c.z() })
            };
            const Math::cVec3f maximum{
                std::max({ _rCollider.a.x(), _rCollider.b.x(), _rCollider.c.x() }),
                std::max({ _rCollider.a.y(), _rCollider.b.y(), _rCollider.c.y() }),
                std::max({ _rCollider.a.z(), _rCollider.b.z(), _rCollider.c.z() })
            };
            const sColliderHandle handle = AddCollider(sAABBCollider{ .center = (minimum + maximum) * 0.5f, .halfExtents = (maximum - minimum) * 0.5f });
            m_triangles[handle.index] = _rCollider;
            return handle;
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
            m_triangles[_handle.index].reset();
            m_freeSlots.push_back(_handle.index);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cCollisionWorld::Clear()
        {
            m_colliders.clear();
            m_triangles.clear();
            m_generations.clear();
            m_freeSlots.clear();
            m_spatialGrid.clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f cCollisionWorld::MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement, float _maximumStepHeight)
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
                        if (m_triangles[colliderIndex])
                        {
                            const Math::cVec3f offset = movedCapsule.center - collider.center;
                            if (std::abs(offset.x()) > collider.halfExtents.x() + movedCapsule.radius
                                || std::abs(offset.z()) > collider.halfExtents.z() + movedCapsule.radius
                                || std::abs(offset.y()) > collider.halfExtents.y() + movedCapsule.halfHeight + movedCapsule.radius)
                            {
                                continue;
                            }

                            const auto& triangle = *m_triangles[colliderIndex];
                            const Math::cVec3f normal = (triangle.b - triangle.a).cross(triangle.c - triangle.a).normalized();
                            // Ground snapping handles reachable slopes and low ledges during horizontal movement.
                            if (_maximumStepHeight > 0.0f && _rMovement.y() == 0.0f && normal.y() >= 0.5f)
                            {
                                const float planeHeight = triangle.a.y()
                                    - (normal.x() * (movedCapsule.center.x() - triangle.a.x())
                                    + normal.z() * (movedCapsule.center.z() - triangle.a.z())) / normal.y();
                                const float feet = movedCapsule.center.y() - movedCapsule.halfHeight - movedCapsule.radius;
                                if (planeHeight <= feet + _maximumStepHeight && normal.dot(movedCapsule.center - triangle.a) > 0.0f)
                                {
                                    continue;
                                }
                            }
                            if (!IntersectCapsuleTriangle(movedCapsule, triangle, result))
                            {
                                continue;
                            }
                        }
                        else if (!IntersectCapsuleAABB(movedCapsule, collider, result))
                        {
                            continue;
                        }

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
                if (m_triangles[colliderIndex])
                {
                    float height = 0.0f;
                    if (FindTriangleGroundHeight(_rPosition, *m_triangles[colliderIndex], height) && height <= _maximumHeight)
                    {
                        bestHeight = std::max(bestHeight, height);
                    }
                    continue;
                }
                if (!collider.isGround)
                    continue;

                const Math::cVec3f minimum = collider.center - collider.halfExtents;
                const Math::cVec3f maximum = collider.center + collider.halfExtents;
                if (_rPosition.x() < minimum.x() || _rPosition.x() > maximum.x() ||
                    _rPosition.z() < minimum.z() || _rPosition.z() > maximum.z())
                    continue;

                const float surfaceHeight = collider.groundHeightSampler
                    ? collider.groundHeightSampler(_rPosition.x(), _rPosition.z()) + maximum.y()
                    : maximum.y();
                if (surfaceHeight <= _maximumHeight)
                    bestHeight = std::max(bestHeight, surfaceHeight);
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

            std::vector<std::size_t> candidates{ uniqueCandidates.begin(), uniqueCandidates.end() };
            std::sort(candidates.begin(), candidates.end());
            return candidates;
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

        sColliderHandle AddCollider(const sTriangleCollider& _rCollider)
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

        Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement, float _maximumStepHeight)
        {
            return cCollisionWorld::GetInstance().MoveCapsule(_rCapsule, _rMovement, _maximumStepHeight);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement)
        {
            return cCollisionWorld::GetInstance().MoveCapsule(_rCapsule, _rMovement, 0.0f);
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


