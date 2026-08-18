#include "reflectionProbeManager.h"

#include "graphics/vulkan/reflectionProbe.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        struct sCellCoord
        {
            int32_t x = 0;
            int32_t z = 0;

            bool operator==(const sCellCoord& _rOther) const
            {
                return x == _rOther.x && z == _rOther.z;
            }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        struct sCellCoordHash
        {
            size_t operator()(const sCellCoord& _rCell) const
            {
                const size_t xHash = std::hash<int32_t>{}(_rCell.x);
                const size_t zHash = std::hash<int32_t>{}(_rCell.z);

                return xHash ^ (zHash << 1);
            }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        struct sProbeCandidate
        {
            ReflectionProbeHandle probeHandle;
            float distanceToBoxSquared = 0.0f;
            float distanceToProbeSquared = 0.0f;
        };

        // -------------------------------------------------------------------------------------------------------------------------

        class cReflectionProbeManager
        {

        public:

            static cReflectionProbeManager& Get();

            ReflectionProbeHandle AddProbe(const sReflectionProbe& _rProbe);
            void SetProbe(ReflectionProbeHandle _probeHandle, const sReflectionProbe& _rProbe);

            void Clear();

            void SetCellSize(float _cellSize);
            float GetCellSize() const;

            uint32_t GetProbeCount() const;

            const sReflectionProbe& GetProbe(ReflectionProbeHandle _probeHandle) const;
            const std::vector<sReflectionProbe>& GetProbes() const;

            std::vector<ReflectionProbeHandle> FindActiveProbeIndices(const Math::cVec3f& _rPosition, uint32_t _maxProbeCount, int32_t _searchRadiusCells) const;

            void SetProbeDirty(ReflectionProbeHandle _probeHandle, bool _dirty);

        private:

            cReflectionProbeManager();
            ~cReflectionProbeManager();

            cReflectionProbeManager(const cReflectionProbeManager&) = delete;
            const cReflectionProbeManager& operator=(const cReflectionProbeManager&) = delete;

            cReflectionProbeManager(cReflectionProbeManager&&) = delete;
            cReflectionProbeManager& operator=(cReflectionProbeManager&&) = delete;

            sCellCoord GetCellCoord(float _x, float _z) const;

            void InsertProbeIntoGrid(ReflectionProbeHandle _probeHandle);
            void RebuildGrid();

            float CalculateDistanceToBoxSquared(const Math::cVec3f& _rPosition, const sReflectionProbe& _rProbe) const;
            float CalculateDistanceToProbeSquared(const Math::cVec3f& _rPosition, const sReflectionProbe& _rProbe) const;

            uint32_t GetProbeIndex(ReflectionProbeHandle _probeHandle) const;

        private:

            float m_cellSize = 32.0f;

            std::vector<sReflectionProbe> m_probes;

            std::unordered_map<sCellCoord, std::vector<ReflectionProbeHandle>, sCellCoordHash> m_grid;
        };

        // -------------------------------------------------------------------------------------------------------------------------

        cReflectionProbeManager& cReflectionProbeManager::Get()
        {
            static cReflectionProbeManager manager;

            return manager;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        ReflectionProbeHandle cReflectionProbeManager::AddProbe(const sReflectionProbe& _rProbe)
        {
            const ReflectionProbeHandle probeHandle = static_cast<ReflectionProbeHandle>(m_probes.size());

            m_probes.push_back(_rProbe);

            InsertProbeIntoGrid(probeHandle);

            return probeHandle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cReflectionProbeManager::SetProbe(ReflectionProbeHandle _probeHandle, const sReflectionProbe& _rProbe)
        {
            const uint32_t probeIndex = GetProbeIndex(_probeHandle);

            m_probes[probeIndex] = _rProbe;

            RebuildGrid();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cReflectionProbeManager::Clear()
        {
            m_probes.clear();
            m_grid.clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cReflectionProbeManager::SetCellSize(float _cellSize)
        {
            if (_cellSize <= 0.0f)
            {
                throw std::runtime_error("Reflection probe cell size must be greater than zero!");
            }

            m_cellSize = _cellSize;

            RebuildGrid();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float cReflectionProbeManager::GetCellSize() const
        {
            return m_cellSize;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        uint32_t cReflectionProbeManager::GetProbeCount() const
        {
            return static_cast<uint32_t>(m_probes.size());
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sReflectionProbe& cReflectionProbeManager::GetProbe(ReflectionProbeHandle _probeHandle) const
        {
            return m_probes[GetProbeIndex(_probeHandle)];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const std::vector<sReflectionProbe>& cReflectionProbeManager::GetProbes() const
        {
            return m_probes;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<ReflectionProbeHandle> cReflectionProbeManager::FindActiveProbeIndices(
            const Math::cVec3f& _rPosition,
            uint32_t _maxProbeCount,
            int32_t _searchRadiusCells) const
        {
            std::vector<ReflectionProbeHandle> activeProbeHandles;

            if (_maxProbeCount == 0 || m_probes.empty())
            {
                return activeProbeHandles;
            }

            _searchRadiusCells = std::max(_searchRadiusCells, 0);

            const sCellCoord centerCell = GetCellCoord(_rPosition.x(), _rPosition.z());

            std::vector<bool> visited(m_probes.size(), false);
            std::vector<sProbeCandidate> candidates;

            for (int32_t offsetZ = -_searchRadiusCells; offsetZ <= _searchRadiusCells; ++offsetZ)
            {
                for (int32_t offsetX = -_searchRadiusCells; offsetX <= _searchRadiusCells; ++offsetX)
                {
                    const sCellCoord cell =
                    {
                        centerCell.x + offsetX,
                        centerCell.z + offsetZ
                    };

                    const auto cellIterator = m_grid.find(cell);

                    if (cellIterator == m_grid.end())
                    {
                        continue;
                    }

                    for (ReflectionProbeHandle probeHandle : cellIterator->second)
                    {
                        const uint32_t probeIndex = static_cast<uint32_t>(probeHandle);

                        if (probeIndex >= m_probes.size() || visited[probeIndex])
                        {
                            continue;
                        }

                        visited[probeIndex] = true;

                        const sReflectionProbe& rProbe = m_probes[probeIndex];

                        sProbeCandidate candidate{};

                        candidate.probeHandle               = probeHandle;
                        candidate.distanceToBoxSquared      = CalculateDistanceToBoxSquared(_rPosition, rProbe);
                        candidate.distanceToProbeSquared    = CalculateDistanceToProbeSquared(_rPosition, rProbe);

                        candidates.push_back(candidate);
                    }
                }
            }

            std::sort(
                candidates.begin(),
                candidates.end(),
                [](const sProbeCandidate& _rA, const sProbeCandidate& _rB)
                {
                    if (_rA.distanceToBoxSquared != _rB.distanceToBoxSquared)
                    {
                        return _rA.distanceToBoxSquared < _rB.distanceToBoxSquared;
                    }

                    return _rA.distanceToProbeSquared < _rB.distanceToProbeSquared;
                }
            );

            const uint32_t activeProbeCount = std::min(_maxProbeCount, static_cast<uint32_t>(candidates.size()));

            activeProbeHandles.reserve(activeProbeCount);

            for (uint32_t index = 0; index < activeProbeCount; ++index)
            {
                activeProbeHandles.push_back(candidates[index].probeHandle);
            }

            return activeProbeHandles;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cReflectionProbeManager::SetProbeDirty(ReflectionProbeHandle _probeHandle, bool _dirty)
        {
            m_probes[GetProbeIndex(_probeHandle)].dirty = _dirty;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sCellCoord cReflectionProbeManager::GetCellCoord(float _x, float _z) const
        {
            return
            {
                static_cast<int32_t>(std::floor(_x / m_cellSize)),
                static_cast<int32_t>(std::floor(_z / m_cellSize))
            };
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cReflectionProbeManager::InsertProbeIntoGrid(ReflectionProbeHandle _probeHandle)
        {
            const uint32_t probeIndex = GetProbeIndex(_probeHandle);

            const sReflectionProbe& rProbe = m_probes[probeIndex];

            const float minX = std::min(rProbe.boxMin.x(), rProbe.boxMax.x());
            const float minZ = std::min(rProbe.boxMin.z(), rProbe.boxMax.z());

            const float maxX = std::max(rProbe.boxMin.x(), rProbe.boxMax.x());
            const float maxZ = std::max(rProbe.boxMin.z(), rProbe.boxMax.z());

            const sCellCoord minCell = GetCellCoord(minX, minZ);
            const sCellCoord maxCell = GetCellCoord(maxX, maxZ);

            for (int32_t cellZ = minCell.z; cellZ <= maxCell.z; ++cellZ)
            {
                for (int32_t cellX = minCell.x; cellX <= maxCell.x; ++cellX)
                {
                    const sCellCoord cell =
                    {
                        cellX,
                        cellZ
                    };

                    m_grid[cell].push_back(_probeHandle);
                }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cReflectionProbeManager::RebuildGrid()
        {
            m_grid.clear();

            for (uint32_t probeIndex = 0; probeIndex < static_cast<uint32_t>(m_probes.size()); ++probeIndex)
            {
                InsertProbeIntoGrid(static_cast<ReflectionProbeHandle>(probeIndex));
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float cReflectionProbeManager::CalculateDistanceToBoxSquared(const Math::cVec3f& _rPosition, const sReflectionProbe& _rProbe) const
        {
            const float minX = std::min(_rProbe.boxMin.x(), _rProbe.boxMax.x());
            const float minY = std::min(_rProbe.boxMin.y(), _rProbe.boxMax.y());
            const float minZ = std::min(_rProbe.boxMin.z(), _rProbe.boxMax.z());

            const float maxX = std::max(_rProbe.boxMin.x(), _rProbe.boxMax.x());
            const float maxY = std::max(_rProbe.boxMin.y(), _rProbe.boxMax.y());
            const float maxZ = std::max(_rProbe.boxMin.z(), _rProbe.boxMax.z());

            float distanceX = 0.0f;
            float distanceY = 0.0f;
            float distanceZ = 0.0f;

            if (_rPosition.x() < minX)
            {
                distanceX = minX - _rPosition.x();
            }
            else if (_rPosition.x() > maxX)
            {
                distanceX = _rPosition.x() - maxX;
            }

            if (_rPosition.y() < minY)
            {
                distanceY = minY - _rPosition.y();
            }
            else if (_rPosition.y() > maxY)
            {
                distanceY = _rPosition.y() - maxY;
            }

            if (_rPosition.z() < minZ)
            {
                distanceZ = minZ - _rPosition.z();
            }
            else if (_rPosition.z() > maxZ)
            {
                distanceZ = _rPosition.z() - maxZ;
            }

            return distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float cReflectionProbeManager::CalculateDistanceToProbeSquared(const Math::cVec3f& _rPosition, const sReflectionProbe& _rProbe) const
        {
            const float distanceX = _rPosition.x() - _rProbe.position.x();
            const float distanceY = _rPosition.y() - _rProbe.position.y();
            const float distanceZ = _rPosition.z() - _rProbe.position.z();

            return distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        uint32_t cReflectionProbeManager::GetProbeIndex(ReflectionProbeHandle _probeHandle) const
        {
            const uint32_t probeIndex = static_cast<uint32_t>(_probeHandle);

            if (probeIndex >= m_probes.size())
            {
                throw std::runtime_error("Invalid reflection probe handle!");
            }

            return probeIndex;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cReflectionProbeManager::cReflectionProbeManager()
            : m_cellSize(32.0f)
            , m_probes()
            , m_grid()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cReflectionProbeManager::~cReflectionProbeManager()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------
    // Public ReflectionProbeManager API
    // -------------------------------------------------------------------------------------------------------------------------
    namespace ReflectionProbeManager
    {
        ReflectionProbeHandle AddProbe(const sReflectionProbe& _rProbe)
        {
            return cReflectionProbeManager::Get().AddProbe(_rProbe);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void SetProbe(ReflectionProbeHandle _probeHandle, const sReflectionProbe& _rProbe)
        {
            cReflectionProbeManager::Get().SetProbe(_probeHandle, _rProbe);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void Clear()
        {
            cReflectionProbeManager::Get().Clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void SetCellSize(float _cellSize)
        {
            cReflectionProbeManager::Get().SetCellSize(_cellSize);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        float GetCellSize()
        {
            return cReflectionProbeManager::Get().GetCellSize();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        uint32_t GetProbeCount()
        {
            return cReflectionProbeManager::Get().GetProbeCount();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sReflectionProbe& GetProbe(ReflectionProbeHandle _probeHandle)
        {
            return cReflectionProbeManager::Get().GetProbe(_probeHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const std::vector<sReflectionProbe>& GetProbes()
        {
            return cReflectionProbeManager::Get().GetProbes();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<ReflectionProbeHandle> FindActiveProbeIndices(const Math::cVec3f& _rPosition, uint32_t _maxProbeCount, int32_t _searchRadiusCells)
        {
            return cReflectionProbeManager::Get().FindActiveProbeIndices(_rPosition, _maxProbeCount, _searchRadiusCells);
        }

        void SetProbeDirty(ReflectionProbeHandle _probeHandle, bool _dirty)
        {
            cReflectionProbeManager::Get().SetProbeDirty(_probeHandle, _dirty);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }
    
    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------

