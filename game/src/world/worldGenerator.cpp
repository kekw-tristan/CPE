#include "worldGenerator.h"
#include "chunk.h"
#include "worldConfig.h"
#include "mushroomDungeon.h"
#include "worldModels.h"
#include "terrainHeight.h"
#include "biome/forestGenerator.h"
#include "../spells/spellManager.h"
#include "graphics/scene/scene.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"
#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "math/matrix4x4.h"
#include "physics/collisionWorld.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <random>

using namespace Engine;

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{
    namespace
    {
        Math::cMatrix4x4f CreateCollisionTransform(const GFX::sTransform& _rTransform)
        {
            using Math::cMatrix4x4f;
            const cMatrix4x4f rotation = cMatrix4x4f::rotationX(_rTransform.rotation.x())
                * cMatrix4x4f::rotationY(_rTransform.rotation.y()) * cMatrix4x4f::rotationZ(_rTransform.rotation.z());
            return cMatrix4x4f::scale(_rTransform.scale) * rotation * cMatrix4x4f::translation(_rTransform.position);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void AddPrimitiveColliders(sLoadedChunk& _rChunk)
        {
            for (const auto& instance : _rChunk.scene.GetShapeInstances())
            {
                if (instance.collisionMode == GFX::eShapeCollisionMode::Disabled)
                    continue;

                const auto& model = GFX::ShapeModelManager::GetShapeModel(instance.modelHandle);
                const Math::cMatrix4x4f instanceMatrix = CreateCollisionTransform(instance.transform);
                for (const auto& part : model.shapes)
                {
                    // Existing biome collision proxies remain responsible for the original primitives.
                    switch (part.meshType)
                    {
                        case GFX::sMeshTypes::Cube:
                        case GFX::sMeshTypes::Pyramid:
                        case GFX::sMeshTypes::Sphere:
                        case GFX::sMeshTypes::Cylinder:
                        case GFX::sMeshTypes::Cone:
                        case GFX::sMeshTypes::Torus:
                        case GFX::sMeshTypes::Crystal:
                            if (instance.collisionMode != GFX::eShapeCollisionMode::Mesh)
                                continue;
                            break;

                        case GFX::sMeshTypes::BeveledCube:
                        case GFX::sMeshTypes::Frustum:
                        case GFX::sMeshTypes::Wedge:
                        case GFX::sMeshTypes::TriangularPrism:
                        case GFX::sMeshTypes::IcoSphere:
                        case GFX::sMeshTypes::Rock:
                        case GFX::sMeshTypes::GrassBlade:
                        case GFX::sMeshTypes::Capsule:
                        case GFX::sMeshTypes::Arch:
                        case GFX::sMeshTypes::ExtrudedPolygon:
                        case GFX::sMeshTypes::Disc:
                        case GFX::sMeshTypes::Triangle:
                        case GFX::sMeshTypes::Arc:
                            break;

                        default:
                            continue;
                    }

                    const auto& mesh = GFX::ShapeMeshLibrary::GetMeshData(part.meshType);
                    const Math::cMatrix4x4f matrix = CreateCollisionTransform(part.transform) * instanceMatrix;
                    const Math::cVec3f x = matrix.transformDirection({ 1.0f, 0.0f, 0.0f });
                    const Math::cVec3f y = matrix.transformDirection({ 0.0f, 1.0f, 0.0f });
                    const Math::cVec3f z = matrix.transformDirection({ 0.0f, 0.0f, 1.0f });
                    const bool mirrored = x.cross(y).dot(z) < 0.0f;
                    for (size_t index = 0; index < mesh.indices.size(); index += 3)
                    {
                        Physics::sTriangleCollider triangle{
                            matrix.transformPoint(mesh.vertices[mesh.indices[index]].position),
                            matrix.transformPoint(mesh.vertices[mesh.indices[index + 1]].position),
                            matrix.transformPoint(mesh.vertices[mesh.indices[index + 2]].position)
                        };
                        if (mirrored)
                        {
                            std::swap(triangle.b, triangle.c);
                        }
                        // A zero scale can collapse edited model parts to lines or points.
                        if ((triangle.b - triangle.a).cross(triangle.c - triangle.a).lengthSquared() <= 0.0f)
                        {
                            continue;
                        }
                        _rChunk.colliders.push_back(Physics::CollisionWorld::AddCollider(triangle));
                    }
                }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void AddReflectionProbe(sLoadedChunk& _rChunk, const sChunk& _rChunkData)
        {
            constexpr float c_probeHeight = 7.0f;
            constexpr float c_halfChunkSize = static_cast<float>(c_chunkSize) * 0.5f;

            const float worldX = static_cast<float>(_rChunkData.coordinate.x * c_chunkSize);
            const float worldZ = static_cast<float>(_rChunkData.coordinate.z * c_chunkSize);

            const float terrainHeight = _rChunkData.height + GetTerrainSurfaceHeight(worldX, worldZ);

            sReflectionProbeDesc probe{};
            probe.blendDistance = 10.0f;
            // Overlap neighboring influence volumes throughout the edge fade.
            const float probeHalfExtent = c_halfChunkSize + probe.blendDistance;
            probe.position = { worldX, terrainHeight + c_probeHeight, worldZ };
            probe.boxMin = { worldX - probeHalfExtent, terrainHeight - 48.0f, worldZ - probeHalfExtent };
            probe.boxMax = { worldX + probeHalfExtent, terrainHeight + 64.0f, worldZ + probeHalfExtent };
            probe.resolution = 64;

            _rChunk.reflectionProbes.push_back(probe);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        class cWorldGenerator
        {
            public:

                int m_seed    = 0;
                int m_centerX = 0;
                int m_centerZ = 0;
                bool m_hasCenter = false;

                sWorldLayout m_layout;
                std::map<std::pair<int, int>, sLoadedChunk> m_chunks;

                bool m_windowComplete = false;

                void GenerateLayout()
                {
                    std::mt19937 randomGenerator(m_seed);

                    m_layout.mainPath.clear();
                    std::uniform_real_distribution<float> angleOffset(-0.16f, 0.16f);

                    for (size_t i = 0; i < m_layout.dungeons.size(); ++i)
                    {
                        const float angle   = 0.785398f + static_cast<float>(i) * 1.570796f + angleOffset(randomGenerator);
                        auto& dungeon       = m_layout.dungeons[i];
                        dungeon.center      = Math::cVec3f(std::cos(angle) * c_dungeonRadius, 0.0f, std::sin(angle) * c_dungeonRadius);
                        dungeon.bossId      = static_cast<sBossId::Enum>(i);
                        dungeon.type        = Gameplay::SpellManager::GetBoss(dungeon.bossId).enemyType;

                        const bool mushroomDungeon = dungeon.bossId == sBossId::ForestSporecap;
                        const bool cageDungeon = dungeon.bossId == sBossId::ForestCrawler;
                        const bool treeDungeon = dungeon.bossId == sBossId::ForestBrute;
                        const bool acornDungeon = dungeon.bossId == sBossId::ForestThornwolf;
                        if (mushroomDungeon)
                        {
                            // The module layout is retained with the world layout before any chunk is streamed.
                            const float x = std::cos(angle) * c_sporecapDungeonRadius;
                            const float z = std::sin(angle) * c_sporecapDungeonRadius;
                            dungeon.center = Math::cVec3f(x, 0.0f, z);
                            GenerateMushroomDungeonLayout(m_seed, dungeon);
                            float floorHeight = GetTerrainSurfaceHeight(x, z);
                            for (float localZ = dungeon.mushroomLayout.minimumBounds.z(); localZ <= dungeon.mushroomLayout.maximumBounds.z(); localZ += 4.0f)
                            {
                                for (float localX = dungeon.mushroomLayout.minimumBounds.x(); localX <= dungeon.mushroomLayout.maximumBounds.x(); localX += 4.0f)
                                    floorHeight = std::max(floorHeight, GetTerrainSurfaceHeight(x + localX, z + localZ));
                            }
                            dungeon.center = Math::cVec3f(x, floorHeight + 2.0f, z);
                        }
                        else
                        {
                            // The expanded aviary needs its own reservation outside the spawn
                            // clearing. Preserve its local cage origin and survey every wing.
                            if (cageDungeon)
                            {
                                dungeon.center = { std::cos(angle) * c_cageDungeonRadius, 0.0f,
                                    std::sin(angle) * c_cageDungeonRadius };
                            }
                            if (treeDungeon)
                            {
                                dungeon.center = { std::cos(angle) * c_treeDungeonRadius, 0.0f,
                                    std::sin(angle) * c_treeDungeonRadius };
                            }
                            if (acornDungeon)
                            {
                                dungeon.center = { std::cos(angle) * c_acornDungeonRadius, 0.0f,
                                    std::sin(angle) * c_acornDungeonRadius };
                            }
                            const float front = cageDungeon ? c_cageDungeonFront
                                : (acornDungeon ? c_acornDungeonFront : c_bossDungeonFront);
                            const float back = cageDungeon ? c_cageDungeonBack
                                : (treeDungeon ? c_treeDungeonBack : (acornDungeon ? c_acornDungeonBack : c_bossDungeonBack));
                            const float halfWidth = cageDungeon ? c_cageDungeonHalfWidth
                                : (treeDungeon ? c_treeDungeonHalfWidth : (acornDungeon ? c_acornDungeonHalfWidth : c_bossDungeonHalfWidth));
                            float floorHeight = GetTerrainSurfaceHeight(dungeon.center.x(), dungeon.center.z());
                            for (float z = front; z <= back; z += 2.0f)
                            {
                                for (float x = -halfWidth; x <= halfWidth; x += 2.0f)
                                    floorHeight = std::max(floorHeight,
                                        GetTerrainSurfaceHeight(dungeon.center.x() + x, dungeon.center.z() + z));
                            }
                            dungeon.center = { dungeon.center.x(), floorHeight + 2.0f, dungeon.center.z() };
                        }
                        const float approach = cageDungeon ? c_cageDungeonApproach
                            : (acornDungeon ? c_acornDungeonApproach : c_bossDungeonApproach);
                        const float entranceZ = mushroomDungeon ? -dungeon.mushroomLayout.minimumBounds.z() - 8.0f * c_mushroomDungeonScale : -approach;
                        const float approachZ = entranceZ + 4.0f;

                        m_layout.mainPath.push_back({ Math::cVec3f(0.0f, 0.0f, 0.0f) });
                        m_layout.mainPath.push_back({ Math::cVec3f(dungeon.center.x() * 0.4f, 0.0f, dungeon.center.z() - approachZ) });
                        m_layout.mainPath.push_back({ dungeon.center + Math::cVec3f(0.0f, 0.0f, -approachZ) });
                        m_layout.mainPath.push_back({ dungeon.center + Math::cVec3f(0.0f, 0.0f, -entranceZ) });
                        m_layout.mainPath.push_back({ dungeon.center + Math::cVec3f(0.0f, 0.0f, -approachZ) });
                        m_layout.mainPath.push_back({ Math::cVec3f(dungeon.center.x() * 0.4f, 0.0f, dungeon.center.z() - approachZ) });
                        m_layout.mainPath.push_back({ Math::cVec3f(0.0f, 0.0f, 0.0f) });
                    }
                }
        };

        cWorldGenerator& GetGenerator()
        {
            static cWorldGenerator generator;
            return generator;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace WorldGenerator
    {
        void Generate(int _seed)
        {
            auto& generator = GetGenerator();
            generator.m_seed = _seed;
            Clear();
            generator.m_hasCenter = false;

            if (!WorldModels::Load("./assets/models"))
                std::cerr << "One or more world models could not be loaded.\n";

            generator.GenerateLayout();
            Update(Math::cVec3f(0.0f, 0.0f, 0.0f), (2 * c_chunkLoadRadius + 1) * (2 * c_chunkLoadRadius + 1));
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool Update(const Math::cVec3f& _rPosition, int _chunkBudget)
        {
            auto& generator = GetGenerator();

            int centerX = static_cast<int>(std::floor(_rPosition.x() / c_chunkSize + 0.5f));
            int centerZ = static_cast<int>(std::floor(_rPosition.z() / c_chunkSize + 0.5f));

            // Avoid repeatedly unloading/reloading whole rows when combat movement straddles a chunk edge.
            constexpr float c_streamingHysteresis = 8.0f;
            const float centerMargin = c_chunkSize * 0.5f + c_streamingHysteresis;
            if (generator.m_hasCenter)
            {
                if (std::abs(_rPosition.x() - generator.m_centerX * c_chunkSize) <= centerMargin)
                    centerX = generator.m_centerX;
                if (std::abs(_rPosition.z() - generator.m_centerZ * c_chunkSize) <= centerMargin)
                    centerZ = generator.m_centerZ;
            }

            const bool moved = !generator.m_hasCenter || centerX != generator.m_centerX || centerZ != generator.m_centerZ;

            if (!moved && generator.m_windowComplete)
                return false;

            generator.m_hasCenter = true;
            generator.m_centerX   = centerX;
            generator.m_centerZ   = centerZ;

            bool changed = false;

            // The Elder Shell extends beyond the ordinary room streaming window.
            // Retain its single owner (and colliders) while its envelope is nearby.
            const auto& mushroom = generator.m_layout.dungeons[sBossId::ForestSporecap];
            const auto& bounds = mushroom.mushroomLayout;
            constexpr float c_landmarkViewMargin = c_chunkSize * 2.0f;
            const bool retainLandmark = !bounds.modules.empty()
                && centerX * c_chunkSize >= mushroom.center.x() + bounds.minimumBounds.x() - c_landmarkViewMargin
                && centerX * c_chunkSize <= mushroom.center.x() + bounds.maximumBounds.x() + c_landmarkViewMargin
                && centerZ * c_chunkSize >= mushroom.center.z() + bounds.minimumBounds.z() - c_landmarkViewMargin
                && centerZ * c_chunkSize <= mushroom.center.z() + bounds.maximumBounds.z() + c_landmarkViewMargin;
            const std::pair<int, int> landmarkOwner =
            {
                static_cast<int>(std::floor(mushroom.center.x() / c_chunkSize + 0.5f)),
                static_cast<int>(std::floor(mushroom.center.z() / c_chunkSize + 0.5f))
            };

            if (moved)
            {
                std::erase_if(generator.m_chunks, [&](const auto& _rEntry)
                {
                    if (std::abs(_rEntry.first.first - centerX) <= c_chunkLoadRadius
                        && std::abs(_rEntry.first.second - centerZ) <= c_chunkLoadRadius)
                        return false;

                    if (retainLandmark && _rEntry.first == landmarkOwner)
                        return false;

                    for (auto handle : _rEntry.second.colliders)
                        Physics::CollisionWorld::RemoveCollider(handle);

                    changed = true;
                    return true;
                });
            }

            generator.m_windowComplete = true;
            int generated = 0;

            // Ground first, then the landmark owner, then normal distance rings.
            // The extra owner still consumes the existing per-update chunk budget.
            for (int pass = 0; pass <= c_chunkLoadRadius + 1; ++pass)
            {
                const bool landmarkPass = pass == 1;
                if (landmarkPass && !retainLandmark)
                    continue;
                const int radius = pass == 0 ? 0 : pass - 1;
                const int firstZ = landmarkPass ? landmarkOwner.second : centerZ - radius;
                const int lastZ = landmarkPass ? landmarkOwner.second : centerZ + radius;
                const int firstX = landmarkPass ? landmarkOwner.first : centerX - radius;
                const int lastX = landmarkPass ? landmarkOwner.first : centerX + radius;
                for (int z = firstZ; z <= lastZ; ++z)
                {
                    for (int x = firstX; x <= lastX; ++x)
                    {
                        if ((!landmarkPass && std::max(std::abs(x - centerX), std::abs(z - centerZ)) != radius)
                            || x < -c_worldChunkCountX / 2 || x >= c_worldChunkCountX / 2
                            || z < -c_worldChunkCountZ / 2 || z >= c_worldChunkCountZ / 2
                            || generator.m_chunks.contains({ x, z }))
                            continue;

                        if (generated >= std::max(1, _chunkBudget))
                        {
                            generator.m_windowComplete = false;
                            continue;
                        }

                        auto& loaded = generator.m_chunks[{ x, z }];
                        sChunk chunk{};
                        chunk.coordinate = { x, 0, z };
                        chunk.biome      = sBiomeType::Forest;

                        std::seed_seq seed{ static_cast<uint32_t>(generator.m_seed),
                            static_cast<uint32_t>(x), static_cast<uint32_t>(z) };
                        std::mt19937 randomGenerator(seed);

                        std::vector<Physics::sAABBCollider> colliders;
                        ForestGenerator::GenerateChunk(loaded.scene, chunk, randomGenerator,
                            generator.m_layout, loaded.spawns, colliders);
                        AddReflectionProbe(loaded, chunk);

                        loaded.colliders.reserve(colliders.size());
                        for (const auto& collider : colliders)
                            loaded.colliders.push_back(Physics::CollisionWorld::AddCollider(collider));

                        AddPrimitiveColliders(loaded);

                        ++generated;
                        changed = true;
                    }
                }
            }

            return changed;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const std::map<std::pair<int, int>, sLoadedChunk>& GetLoadedChunks()
        {
            return GetGenerator().m_chunks;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sWorldLayout& GetLayout()
        {
            return GetGenerator().m_layout;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void Clear()
        {
            auto& generator = GetGenerator();
            generator.m_chunks.clear();
            generator.m_windowComplete = false;
            generator.m_hasCenter = false;
            Physics::CollisionWorld::Clear();
        }
    }
}
