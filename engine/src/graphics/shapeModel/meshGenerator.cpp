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

    sMeshData cMeshGenerator::CreatePlane(const sPlaneDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedPlane";

        const float width = std::max(_rDesc.width, 0.0f);
        const float depth = std::max(_rDesc.depth, 0.0f);

        const float halfWidth = width * 0.5f;
        const float halfDepth = depth * 0.5f;

        const uint32_t segmentsX = std::max(_rDesc.segmentsX, 1);
        const uint32_t segmentsZ = std::max(_rDesc.segmentsZ, 1);

        const uint32_t vertexCountX = segmentsX + 1;
        const uint32_t vertexCountZ = segmentsZ + 1;

        mesh.vertices.reserve(vertexCountX * vertexCountZ);
        mesh.indices.reserve(segmentsX * segmentsZ * 6);

        // -------------------------------------------------------------------------------------------------------------------------
        // Vertices
        // -------------------------------------------------------------------------------------------------------------------------

        for (uint32_t z = 0; z < vertexCountZ; ++z)
        {
            const float zFactor = static_cast<float>(z) / static_cast<float>(segmentsZ);
            const float positionZ = -halfDepth + zFactor * depth;

            for (uint32_t x = 0; x < vertexCountX; ++x)
            {
                const float xFactor = static_cast<float>(x) / static_cast<float>(segmentsX);
                const float positionX = -halfWidth + xFactor * width;

                sVertex vertex{};

                vertex.position = { positionX, 0.0f, positionZ };
                vertex.normal = { 0.0f, 1.0f, 0.0f };
                vertex.uv = { xFactor, zFactor };

                mesh.vertices.push_back(vertex);
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Indices
        // -------------------------------------------------------------------------------------------------------------------------

        for (uint32_t z = 0; z < segmentsZ; ++z)
        {
            for (uint32_t x = 0; x < segmentsX; ++x)
            {
                const uint32_t topLeft = z * vertexCountX + x;
                const uint32_t bottomLeft = (z + 1) * vertexCountX + x;
                const uint32_t bottomRight = (z + 1) * vertexCountX + x + 1;
                const uint32_t topRight = z * vertexCountX + x + 1;

                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(bottomLeft);
                mesh.indices.push_back(bottomRight);

                mesh.indices.push_back(bottomRight);
                mesh.indices.push_back(topRight);
                mesh.indices.push_back(topLeft);
            }
        }

        mesh.bounds = CalculateBounds(mesh.vertices);

        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCube(const sCubeDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCube";

        mesh.vertices.reserve(24);
        mesh.indices.reserve(36);

        const float halfWidth = std::max(_rDesc.width, 0.0f) * 0.5f;
        const float halfHeight = std::max(_rDesc.height, 0.0f) * 0.5f;
        const float halfDepth = std::max(_rDesc.depth, 0.0f) * 0.5f;

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

        const float halfWidth = std::max(_rDesc.width, 0.0f) * 0.5f;
        const float halfHeight = std::max(_rDesc.height, 0.0f) * 0.5f;
        const float halfDepth = std::max(_rDesc.depth, 0.0f) * 0.5f;

        const cVec3f apex{ 0.0f, halfHeight, 0.0f };

        const std::array<cVec3f, 4> basePositions =
        {
            cVec3f{ -halfWidth, -halfHeight,  halfDepth },
            cVec3f{  halfWidth, -halfHeight,  halfDepth },
            cVec3f{  halfWidth, -halfHeight, -halfDepth },
            cVec3f{ -halfWidth, -halfHeight, -halfDepth }
        };

        mesh.vertices.reserve(16);
        mesh.indices.reserve(18);

        for (int sideIndex = 0; sideIndex < 4; ++sideIndex)
        {
            const int nextSideIndex = (sideIndex + 1) % 4;

            const cVec3f& rCurrentPosition = basePositions[sideIndex];
            const cVec3f& rNextPosition = basePositions[nextSideIndex];

            const cVec3f edge = rNextPosition - rCurrentPosition;
            const cVec3f apexDirection = apex - rCurrentPosition;
            const cVec3f normal = edge.cross(apexDirection).normalized();

            const uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

            mesh.vertices.push_back({ .position = rCurrentPosition, .normal = normal, .uv = { 0.0f, 0.0f } });
            mesh.vertices.push_back({ .position = rNextPosition, .normal = normal, .uv = { 1.0f, 0.0f } });
            mesh.vertices.push_back({ .position = apex, .normal = normal, .uv = { 0.5f, 1.0f } });

            mesh.indices.push_back(startIndex + 0);
            mesh.indices.push_back(startIndex + 1);
            mesh.indices.push_back(startIndex + 2);
        }

        const uint32_t baseStartIndex = static_cast<uint32_t>(mesh.vertices.size());
        const cVec3f baseNormal{ 0.0f, -1.0f, 0.0f };

        mesh.vertices.push_back({ .position = basePositions[0], .normal = baseNormal, .uv = { 0.0f, 1.0f } });
        mesh.vertices.push_back({ .position = basePositions[3], .normal = baseNormal, .uv = { 0.0f, 0.0f } });
        mesh.vertices.push_back({ .position = basePositions[2], .normal = baseNormal, .uv = { 1.0f, 0.0f } });
        mesh.vertices.push_back({ .position = basePositions[1], .normal = baseNormal, .uv = { 1.0f, 1.0f } });

        mesh.indices.push_back(baseStartIndex + 0);
        mesh.indices.push_back(baseStartIndex + 1);
        mesh.indices.push_back(baseStartIndex + 2);

        mesh.indices.push_back(baseStartIndex + 2);
        mesh.indices.push_back(baseStartIndex + 3);
        mesh.indices.push_back(baseStartIndex + 0);

        mesh.bounds = CalculateBounds(mesh.vertices);

        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateSphere(const sSphereDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedSphere";

        const float radius = std::max(_rDesc.radius, 0.0f);
        const int segments = std::max(_rDesc.segments, 3);
        const int rings = std::max(_rDesc.rings, 2);

        const int verticesPerRing = segments + 1;

        mesh.vertices.reserve((rings + 1) * verticesPerRing);
        mesh.indices.reserve(rings * segments * 6);

        for (int ringIndex = 0; ringIndex <= rings; ++ringIndex)
        {
            const float v = static_cast<float>(ringIndex) / static_cast<float>(rings);
            const float phi = c_pi * v;

            const float sinPhi = std::sin(phi);
            const float cosPhi = std::cos(phi);

            for (int segmentIndex = 0; segmentIndex <= segments; ++segmentIndex)
            {
                const float u = static_cast<float>(segmentIndex) / static_cast<float>(segments);
                const float theta = 2.0f * c_pi * u;

                const float sinTheta = std::sin(theta);
                const float cosTheta = std::cos(theta);

                const cVec3f normal{ sinPhi * sinTheta, cosPhi, sinPhi * cosTheta };

                sVertex vertex{};

                vertex.position = normal * radius;
                vertex.normal = normal;
                vertex.uv = { u, v };

                mesh.vertices.push_back(vertex);
            }
        }

        for (int ringIndex = 0; ringIndex < rings; ++ringIndex)
        {
            for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
            {
                const uint32_t topLeft = static_cast<uint32_t>(ringIndex * verticesPerRing + segmentIndex);
                const uint32_t bottomLeft = static_cast<uint32_t>((ringIndex + 1) * verticesPerRing + segmentIndex);

                const uint32_t topRight = topLeft + 1;
                const uint32_t bottomRight = bottomLeft + 1;

                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(bottomLeft);
                mesh.indices.push_back(bottomRight);

                mesh.indices.push_back(bottomRight);
                mesh.indices.push_back(topRight);
                mesh.indices.push_back(topLeft);
            }
        }

        mesh.bounds = CalculateBounds(mesh.vertices);

        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCylinder(const sCylinderDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCylinder";

        const float radius = std::max(_rDesc.radius, 0.0f);
        const float halfHeight = std::max(_rDesc.height, 0.0f) * 0.5f;
        const int segments = std::max(_rDesc.segments, 3);

        mesh.vertices.reserve(segments * 4 + 2);
        mesh.indices.reserve(segments * 12);

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const int nextSegmentIndex = (segmentIndex + 1) % segments;

            const float currentFactor = static_cast<float>(segmentIndex) / static_cast<float>(segments);
            const float nextFactor = static_cast<float>(nextSegmentIndex) / static_cast<float>(segments);

            const float currentAngle = currentFactor * 2.0f * c_pi;
            const float nextAngle = nextFactor * 2.0f * c_pi;

            const cVec3f currentNormal{ std::sin(currentAngle), 0.0f, std::cos(currentAngle) };
            const cVec3f nextNormal{ std::sin(nextAngle), 0.0f, std::cos(nextAngle) };

            const cVec3f bottomCurrent{ currentNormal.x() * radius, -halfHeight, currentNormal.z() * radius };
            const cVec3f bottomNext{ nextNormal.x() * radius, -halfHeight, nextNormal.z() * radius };
            const cVec3f topNext{ nextNormal.x() * radius, halfHeight, nextNormal.z() * radius };
            const cVec3f topCurrent{ currentNormal.x() * radius, halfHeight, currentNormal.z() * radius };

            const uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

            mesh.vertices.push_back({ .position = bottomCurrent, .normal = currentNormal, .uv = { currentFactor, 0.0f } });
            mesh.vertices.push_back({ .position = bottomNext, .normal = nextNormal, .uv = { nextFactor, 0.0f } });
            mesh.vertices.push_back({ .position = topNext, .normal = nextNormal, .uv = { nextFactor, 1.0f } });
            mesh.vertices.push_back({ .position = topCurrent, .normal = currentNormal, .uv = { currentFactor, 1.0f } });

            mesh.indices.push_back(startIndex + 0);
            mesh.indices.push_back(startIndex + 1);
            mesh.indices.push_back(startIndex + 2);

            mesh.indices.push_back(startIndex + 2);
            mesh.indices.push_back(startIndex + 3);
            mesh.indices.push_back(startIndex + 0);
        }

        const cVec3f topNormal{ 0.0f, 1.0f, 0.0f };
        const uint32_t topCenterIndex = static_cast<uint32_t>(mesh.vertices.size());

        mesh.vertices.push_back({ .position = { 0.0f, halfHeight, 0.0f }, .normal = topNormal, .uv = { 0.5f, 0.5f } });

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const float factor = static_cast<float>(segmentIndex) / static_cast<float>(segments);
            const float angle = factor * 2.0f * c_pi;

            const float x = std::sin(angle) * radius;
            const float z = std::cos(angle) * radius;

            mesh.vertices.push_back({ .position = { x, halfHeight, z }, .normal = topNormal, .uv = { 0.5f + x / (radius > 0.0f ? 2.0f * radius : 1.0f), 0.5f + z / (radius > 0.0f ? 2.0f * radius : 1.0f) } });
        }

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const uint32_t currentIndex = topCenterIndex + 1 + static_cast<uint32_t>(segmentIndex);
            const uint32_t nextIndex = topCenterIndex + 1 + static_cast<uint32_t>((segmentIndex + 1) % segments);

            mesh.indices.push_back(topCenterIndex);
            mesh.indices.push_back(currentIndex);
            mesh.indices.push_back(nextIndex);
        }

        const cVec3f bottomNormal{ 0.0f, -1.0f, 0.0f };
        const uint32_t bottomCenterIndex = static_cast<uint32_t>(mesh.vertices.size());

        mesh.vertices.push_back({ .position = { 0.0f, -halfHeight, 0.0f }, .normal = bottomNormal, .uv = { 0.5f, 0.5f } });

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const float factor = static_cast<float>(segmentIndex) / static_cast<float>(segments);
            const float angle = factor * 2.0f * c_pi;

            const float x = std::sin(angle) * radius;
            const float z = std::cos(angle) * radius;

            mesh.vertices.push_back({ .position = { x, -halfHeight, z }, .normal = bottomNormal, .uv = { 0.5f + x / (radius > 0.0f ? 2.0f * radius : 1.0f), 0.5f + z / (radius > 0.0f ? 2.0f * radius : 1.0f) } });
        }

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const uint32_t currentIndex = bottomCenterIndex + 1 + static_cast<uint32_t>(segmentIndex);
            const uint32_t nextIndex = bottomCenterIndex + 1 + static_cast<uint32_t>((segmentIndex + 1) % segments);

            mesh.indices.push_back(bottomCenterIndex);
            mesh.indices.push_back(nextIndex);
            mesh.indices.push_back(currentIndex);
        }

        mesh.bounds = CalculateBounds(mesh.vertices);

        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCone(const sConeDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCone";

        const float radius = std::max(_rDesc.radius, 0.0f);
        const float halfHeight = std::max(_rDesc.height, 0.0f) * 0.5f;
        const int segments = std::max(_rDesc.segments, 3);

        const cVec3f apex{ 0.0f, halfHeight, 0.0f };

        mesh.vertices.reserve(segments * 4 + 1);
        mesh.indices.reserve(segments * 6);

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const int nextSegmentIndex = (segmentIndex + 1) % segments;

            const float currentFactor = static_cast<float>(segmentIndex) / static_cast<float>(segments);
            const float nextFactor = static_cast<float>(nextSegmentIndex) / static_cast<float>(segments);

            const float currentAngle = currentFactor * 2.0f * c_pi;
            const float nextAngle = nextFactor * 2.0f * c_pi;

            const cVec3f currentPosition{ std::sin(currentAngle) * radius, -halfHeight, std::cos(currentAngle) * radius };
            const cVec3f nextPosition{ std::sin(nextAngle) * radius, -halfHeight, std::cos(nextAngle) * radius };

            const cVec3f edge = nextPosition - currentPosition;
            const cVec3f apexDirection = apex - currentPosition;
            const cVec3f normal = edge.cross(apexDirection).normalized();

            const uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

            mesh.vertices.push_back({ .position = currentPosition, .normal = normal, .uv = { currentFactor, 0.0f } });
            mesh.vertices.push_back({ .position = nextPosition, .normal = normal, .uv = { nextFactor, 0.0f } });
            mesh.vertices.push_back({ .position = apex, .normal = normal, .uv = { (currentFactor + nextFactor) * 0.5f, 1.0f } });

            mesh.indices.push_back(startIndex + 0);
            mesh.indices.push_back(startIndex + 1);
            mesh.indices.push_back(startIndex + 2);
        }

        const cVec3f baseNormal{ 0.0f, -1.0f, 0.0f };
        const uint32_t baseCenterIndex = static_cast<uint32_t>(mesh.vertices.size());

        mesh.vertices.push_back({ .position = { 0.0f, -halfHeight, 0.0f }, .normal = baseNormal, .uv = { 0.5f, 0.5f } });

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const float factor = static_cast<float>(segmentIndex) / static_cast<float>(segments);
            const float angle = factor * 2.0f * c_pi;

            const float x = std::sin(angle) * radius;
            const float z = std::cos(angle) * radius;

            mesh.vertices.push_back({ .position = { x, -halfHeight, z }, .normal = baseNormal, .uv = { 0.5f + x / (radius > 0.0f ? 2.0f * radius : 1.0f), 0.5f + z / (radius > 0.0f ? 2.0f * radius : 1.0f) } });
        }

        for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
        {
            const uint32_t currentIndex = baseCenterIndex + 1 + static_cast<uint32_t>(segmentIndex);
            const uint32_t nextIndex = baseCenterIndex + 1 + static_cast<uint32_t>((segmentIndex + 1) % segments);

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
            return bounds;

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
            maximumRadiusSquared = std::max(maximumRadiusSquared, difference.lengthSquared());
        }

        bounds.radius = std::sqrt(maximumRadiusSquared);

        return bounds;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------