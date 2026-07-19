#include "meshGenerator.h"

#include "math/vector3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

constexpr float c_pi = 3.1415927f;

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    using Engine::Math::cVec3f;

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCube(const sCubeDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCube";

        mesh.vertices.reserve(24);
        mesh.indices.reserve(36);

        const float halfWidth = _rDesc.width * 0.5f;
        const float halfHeight = _rDesc.height * 0.5f;
        const float halfDepth = _rDesc.depth * 0.5f;

        const std::array<cVec3f, 24> positions =
        {
            // Front +Z
            cVec3f{ -halfWidth, -halfHeight,  halfDepth },
            cVec3f{  halfWidth, -halfHeight,  halfDepth },
            cVec3f{  halfWidth,  halfHeight,  halfDepth },
            cVec3f{ -halfWidth,  halfHeight,  halfDepth },

            // Back -Z
            cVec3f{  halfWidth, -halfHeight, -halfDepth },
            cVec3f{ -halfWidth, -halfHeight, -halfDepth },
            cVec3f{ -halfWidth,  halfHeight, -halfDepth },
            cVec3f{  halfWidth,  halfHeight, -halfDepth },

            // Left -X
            cVec3f{ -halfWidth, -halfHeight, -halfDepth },
            cVec3f{ -halfWidth, -halfHeight,  halfDepth },
            cVec3f{ -halfWidth,  halfHeight,  halfDepth },
            cVec3f{ -halfWidth,  halfHeight, -halfDepth },

            // Right +X
            cVec3f{ halfWidth, -halfHeight,  halfDepth },
            cVec3f{ halfWidth, -halfHeight, -halfDepth },
            cVec3f{ halfWidth,  halfHeight, -halfDepth },
            cVec3f{ halfWidth,  halfHeight,  halfDepth },

            // Top +Y
            cVec3f{ -halfWidth, halfHeight,  halfDepth },
            cVec3f{  halfWidth, halfHeight,  halfDepth },
            cVec3f{  halfWidth, halfHeight, -halfDepth },
            cVec3f{ -halfWidth, halfHeight, -halfDepth },

            // Bottom -Y
            cVec3f{ -halfWidth, -halfHeight, -halfDepth },
            cVec3f{  halfWidth, -halfHeight, -halfDepth },
            cVec3f{  halfWidth, -halfHeight,  halfDepth },
            cVec3f{ -halfWidth, -halfHeight,  halfDepth }
        };

        const std::array<cVec3f, 6> normals =
        {
            cVec3f{  0.0f,  0.0f,  1.0f },
            cVec3f{  0.0f,  0.0f, -1.0f },
            cVec3f{ -1.0f,  0.0f,  0.0f },
            cVec3f{  1.0f,  0.0f,  0.0f },
            cVec3f{  0.0f,  1.0f,  0.0f },
            cVec3f{  0.0f, -1.0f,  0.0f }
        };

        const std::array<std::array<float, 2>, 4> uvs =
        {
            std::array<float, 2>{ 0.0f, 0.0f },
            std::array<float, 2>{ 1.0f, 0.0f },
            std::array<float, 2>{ 1.0f, 1.0f },
            std::array<float, 2>{ 0.0f, 1.0f }
        };

        constexpr int c_numberOfFaces = 6;
        constexpr int c_verticesPerFace = 4;

        for (int faceIndex = 0; faceIndex < c_numberOfFaces; ++faceIndex)
        {
            const uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

            for (int vertexIndex = 0; vertexIndex < c_verticesPerFace; ++vertexIndex)
            {
                const int positionIndex = faceIndex * c_verticesPerFace + vertexIndex;

                sVertex vertex{};

                vertex.position = positions[positionIndex];
                vertex.normal = normals[faceIndex];
                vertex.uv = uvs[vertexIndex];

                mesh.vertices.push_back(vertex);
            }

            mesh.indices.push_back(startIndex + 0);
            mesh.indices.push_back(startIndex + 1);
            mesh.indices.push_back(startIndex + 2);

            mesh.indices.push_back(startIndex + 2);
            mesh.indices.push_back(startIndex + 3);
            mesh.indices.push_back(startIndex + 0);
        }

        mesh.bounds = CalculateBounds(mesh.vertices);

        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreatePyramid(const sPyramidDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedPyramid";

        const int cornerCount = std::max(_rDesc.baseCornerCount, 3);
        const float radius = std::max(_rDesc.baseRadius, 0.0f);
        const float halfHeight = _rDesc.height * 0.5f;

        mesh.vertices.reserve(cornerCount * 4 + 1);
        mesh.indices.reserve(cornerCount * 6);

        const cVec3f apex{ 0.0f, halfHeight, 0.0f };
        const cVec3f baseCenter{ 0.0f, -halfHeight, 0.0f };
        const cVec3f baseNormal{ 0.0f, -1.0f, 0.0f };

        std::vector<cVec3f> basePositions;
        basePositions.reserve(cornerCount);

        for (int cornerIndex = 0; cornerIndex < cornerCount; ++cornerIndex)
        {
            const float factor = static_cast<float>(cornerIndex) / static_cast<float>(cornerCount);
            const float angle = _rDesc.rotationRadians + 2.0f * c_pi * factor;

            const float x = std::sin(angle) * radius;
            const float z = std::cos(angle) * radius;

            basePositions.emplace_back(x, -halfHeight, z);
        }

        for (int cornerIndex = 0; cornerIndex < cornerCount; ++cornerIndex)
        {
            const int nextCornerIndex = (cornerIndex + 1) % cornerCount;

            const cVec3f& rCurrentPosition = basePositions[cornerIndex];
            const cVec3f& rNextPosition = basePositions[nextCornerIndex];

            const cVec3f edge = rNextPosition - rCurrentPosition;
            const cVec3f apexDirection = apex - rCurrentPosition;
            const cVec3f normal = edge.cross(apexDirection).normalized();

            const uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

            sVertex firstVertex{};
            firstVertex.position = rCurrentPosition;
            firstVertex.normal = normal;
            firstVertex.uv = { 0.0f, 0.0f };

            sVertex secondVertex{};
            secondVertex.position = rNextPosition;
            secondVertex.normal = normal;
            secondVertex.uv = { 1.0f, 0.0f };

            sVertex apexVertex{};
            apexVertex.position = apex;
            apexVertex.normal = normal;
            apexVertex.uv = { 0.5f, 1.0f };

            mesh.vertices.push_back(firstVertex);
            mesh.vertices.push_back(secondVertex);
            mesh.vertices.push_back(apexVertex);

            mesh.indices.push_back(startIndex + 0);
            mesh.indices.push_back(startIndex + 1);
            mesh.indices.push_back(startIndex + 2);
        }

        const uint32_t baseCenterIndex = static_cast<uint32_t>(mesh.vertices.size());

        sVertex centerVertex{};
        centerVertex.position = baseCenter;
        centerVertex.normal = baseNormal;
        centerVertex.uv = { 0.5f, 0.5f };

        mesh.vertices.push_back(centerVertex);

        const float inverseDiameter = radius > 0.0f ? 0.5f / radius : 0.0f;

        for (const cVec3f& rPosition : basePositions)
        {
            sVertex vertex{};

            vertex.position = rPosition;
            vertex.normal = baseNormal;
            vertex.uv =
            {
                0.5f + rPosition.x() * inverseDiameter,
                0.5f + rPosition.z() * inverseDiameter
            };

            mesh.vertices.push_back(vertex);
        }

        for (int cornerIndex = 0; cornerIndex < cornerCount; ++cornerIndex)
        {
            const int nextCornerIndex = (cornerIndex + 1) % cornerCount;

            const uint32_t currentIndex = baseCenterIndex + 1 + static_cast<uint32_t>(cornerIndex);
            const uint32_t nextIndex = baseCenterIndex + 1 + static_cast<uint32_t>(nextCornerIndex);

            mesh.indices.push_back(baseCenterIndex);
            mesh.indices.push_back(nextIndex);
            mesh.indices.push_back(currentIndex);
        }

        mesh.bounds = CalculateBounds(mesh.vertices);

        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sBounds cMeshGenerator::CalculateBounds(const std::vector<sVertex>& _rVertices)
    {
        sBounds bounds{};

        if (_rVertices.empty())
        {
            return bounds;
        }

        constexpr float c_floatMax = std::numeric_limits<float>::max();
        constexpr float c_floatMin = std::numeric_limits<float>::lowest();

        float minimumX = c_floatMax;
        float minimumY = c_floatMax;
        float minimumZ = c_floatMax;

        float maximumX = c_floatMin;
        float maximumY = c_floatMin;
        float maximumZ = c_floatMin;

        for (const sVertex& rVertex : _rVertices)
        {
            minimumX = std::min(minimumX, rVertex.position.x());
            minimumY = std::min(minimumY, rVertex.position.y());
            minimumZ = std::min(minimumZ, rVertex.position.z());

            maximumX = std::max(maximumX, rVertex.position.x());
            maximumY = std::max(maximumY, rVertex.position.y());
            maximumZ = std::max(maximumZ, rVertex.position.z());
        }

        bounds.min = cVec3f{ minimumX, minimumY, minimumZ };
        bounds.max = cVec3f{ maximumX, maximumY, maximumZ };
        bounds.center = (bounds.min + bounds.max) * 0.5f;
        bounds.size = bounds.max - bounds.min;

        float maximumRadiusSquared = 0.0f;

        for (const sVertex& rVertex : _rVertices)
        {
            const cVec3f difference = rVertex.position - bounds.center;
            const float distanceSquared = difference.lengthSquared();

            maximumRadiusSquared = std::max(maximumRadiusSquared, distanceSquared);
        }

        bounds.radius = std::sqrt(maximumRadiusSquared);

        return bounds;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------