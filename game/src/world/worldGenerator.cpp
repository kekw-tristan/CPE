#include "worldGenerator.h"
#include "chunk.h"
#include "worldConfig.h"
#include "worldModels.h"
#include "biome/forestGenerator.h"
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
                const auto& model = GFX::ShapeModelManager::GetShapeModel(instance.modelHandle);
                const Math::cMatrix4x4f instanceMatrix = CreateCollisionTransform(instance.transform);
                for (const auto& part : model.shapes)
                {
                    // Existing biome collision proxies remain responsible for the original primitives.
                    switch (part.meshType)
                    {
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

            sReflectionProbeDesc probe{};
            probe.blendDistance = 10.0f;
            // Overlap neighboring influence volumes throughout the edge fade.
            const float probeHalfExtent = c_halfChunkSize + probe.blendDistance;
            probe.position = { worldX, _rChunkData.height + c_probeHeight, worldZ };
            probe.boxMin = { worldX - probeHalfExtent, _rChunkData.height - 12.0f, worldZ - probeHalfExtent };
            probe.boxMax = { worldX + probeHalfExtent, _rChunkData.height + 32.0f, worldZ + probeHalfExtent };
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
                        dungeon.type        = static_cast<sEnemyType::Enum>(i);
                        dungeon.bossId      = static_cast<sBossId::Enum>(i);

                        m_layout.mainPath.push_back({ Math::cVec3f(0.0f, 0.0f, 0.0f) });
                        m_layout.mainPath.push_back({ Math::cVec3f(dungeon.center.x() * 0.4f, 0.0f, dungeon.center.z() - 30.0f) });
                        m_layout.mainPath.push_back({ dungeon.center + Math::cVec3f(0.0f, 0.0f, -30.0f) });
                        m_layout.mainPath.push_back({ dungeon.center + Math::cVec3f(0.0f, 0.0f, -14.0f) });
                        m_layout.mainPath.push_back({ dungeon.center + Math::cVec3f(0.0f, 0.0f, -30.0f) });
                        m_layout.mainPath.push_back({ Math::cVec3f(dungeon.center.x() * 0.4f, 0.0f, dungeon.center.z() - 30.0f) });
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

            const int centerX = static_cast<int>(std::floor(_rPosition.x() / c_chunkSize + 0.5f));
            const int centerZ = static_cast<int>(std::floor(_rPosition.z() / c_chunkSize + 0.5f));

            const bool moved = !generator.m_hasCenter || centerX != generator.m_centerX || centerZ != generator.m_centerZ;

            if (!moved && generator.m_windowComplete)
                return false;

            generator.m_hasCenter = true;
            generator.m_centerX   = centerX;
            generator.m_centerZ   = centerZ;

            bool changed = false;

            if (moved)
            {
                std::erase_if(generator.m_chunks, [&](const auto& _rEntry)
                {
                    if (std::abs(_rEntry.first.first - centerX) <= c_chunkLoadRadius
                        && std::abs(_rEntry.first.second - centerZ) <= c_chunkLoadRadius)
                        return false;

                    for (auto handle : _rEntry.second.colliders)
                        Physics::CollisionWorld::RemoveCollider(handle);

                    changed = true;
                    return true;
                });
            }

            generator.m_windowComplete = true;
            int generated = 0;

            // Nearest chunks first, including the ground after a teleport.
            for (int radius = 0; radius <= c_chunkLoadRadius; ++radius)
            {
                for (int z = centerZ - radius; z <= centerZ + radius; ++z)
                {
                    for (int x = centerX - radius; x <= centerX + radius; ++x)
                    {
                        if (std::max(std::abs(x - centerX), std::abs(z - centerZ)) != radius
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
