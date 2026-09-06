#include "meshGenerator.h"

#include "math/vector3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

constexpr float c_pi = 3.1415927f;

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    using Engine::Math::cVec3f;

    namespace
    {
        void Require(bool _valid)
        {
            if (!_valid)
            {
                throw std::invalid_argument("Invalid procedural mesh descriptor");
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool IsPositive(float _value)
        {
            return std::isfinite(_value) && _value > 0.0f;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void ValidateSegments(int _segments, int _minimum = 3)
        {
            Require(_segments >= _minimum && _segments <= 4096);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void AddFace(sMeshData& _rMesh, std::vector<cVec3f> _positions, const cVec3f& _rOutward)
        {
            cVec3f normal = (_positions[1] - _positions[0]).cross(_positions[2] - _positions[0]).normalized();
            if (normal.dot(_rOutward) < 0.0f)
            {
                std::reverse(_positions.begin(), _positions.end());
                normal = -normal;
            }

            const cVec3f origin = _positions[0];
            const cVec3f tangent = (_positions[1] - origin).normalized();
            const cVec3f bitangent = normal.cross(tangent);
            const uint32_t first = static_cast<uint32_t>(_rMesh.vertices.size());
            for (const cVec3f& rPosition : _positions)
            {
                const cVec3f offset = rPosition - origin;
                _rMesh.vertices.push_back({ .position = rPosition, .normal = normal, .uv = { offset.dot(tangent), offset.dot(bitangent) } });
            }

            for (uint32_t index = 1; index + 1 < _positions.size(); ++index)
            {
                _rMesh.indices.insert(_rMesh.indices.end(), { first, first + index, first + index + 1 });
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        struct sProfilePoint
        {
            float radius;
            float y;
            float normalRadius;
            float normalY;
        };

        void AddLathe(sMeshData& _rMesh, const std::vector<sProfilePoint>& _rProfile, int _segments)
        {
            const uint32_t first = static_cast<uint32_t>(_rMesh.vertices.size());
            const uint32_t stride = static_cast<uint32_t>(_segments + 1);
            for (size_t ring = 0; ring < _rProfile.size(); ++ring)
            {
                const sProfilePoint& rPoint = _rProfile[ring];
                const float v = static_cast<float>(ring) / static_cast<float>(_rProfile.size() - 1);
                for (int segment = 0; segment <= _segments; ++segment)
                {
                    const float u = static_cast<float>(segment) / _segments;
                    const float angle = segment == _segments ? 0.0f : u * 2.0f * c_pi;
                    const cVec3f radial{ std::sin(angle), 0.0f, std::cos(angle) };
                    _rMesh.vertices.push_back({
                        .position = radial * rPoint.radius + cVec3f{ 0.0f, rPoint.y, 0.0f },
                        .normal = (radial * rPoint.normalRadius + cVec3f{ 0.0f, rPoint.normalY, 0.0f }).normalized(),
                        .uv = { u, v }
                    });
                }
            }

            // Profile runs bottom to top. Skip the collapsed triangle at each pole.
            for (uint32_t ring = 0; ring + 1 < _rProfile.size(); ++ring)
            {
                for (int segment = 0; segment < _segments; ++segment)
                {
                    const uint32_t a = first + ring * stride + segment;
                    const uint32_t b = a + stride;
                    if (_rProfile[ring].radius > 0.0f)
                    {
                        _rMesh.indices.insert(_rMesh.indices.end(), { a, a + 1, b });
                    }
                    if (_rProfile[ring + 1].radius > 0.0f)
                    {
                        _rMesh.indices.insert(_rMesh.indices.end(), { a + 1, b + 1, b });
                    }
                }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void AddCap(sMeshData& _rMesh, float _radius, float _y, int _segments, bool _top)
        {
            if (_radius == 0.0f)
            {
                return;
            }

            const uint32_t first = static_cast<uint32_t>(_rMesh.vertices.size());
            const cVec3f normal{ 0.0f, _top ? 1.0f : -1.0f, 0.0f };
            _rMesh.vertices.push_back({ .position = { 0.0f, _y, 0.0f }, .normal = normal, .uv = { 0.5f, 0.5f } });
            for (int segment = 0; segment < _segments; ++segment)
            {
                const float angle = static_cast<float>(segment) / _segments * 2.0f * c_pi;
                const float x = std::sin(angle);
                const float z = std::cos(angle);
                _rMesh.vertices.push_back({ .position = { x * _radius, _y, z * _radius }, .normal = normal, .uv = { 0.5f + x * 0.5f, 0.5f + z * 0.5f } });
                const uint32_t a = first + 1 + segment;
                const uint32_t b = first + 1 + (segment + 1) % _segments;
                _rMesh.indices.insert(_rMesh.indices.end(), { first, _top ? a : b, _top ? b : a });
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sMeshData CreateGeodesic(float _radius, int _subdivisions, float _roughness, uint32_t _seed)
        {
            Require(IsPositive(_radius) && _subdivisions >= 0 && _subdivisions <= 6);
            Require(std::isfinite(_roughness) && _roughness >= 0.0f && _roughness < 1.0f);

            const float goldenRatio = (1.0f + std::sqrt(5.0f)) * 0.5f;
            std::vector<cVec3f> positions =
            {
                { -1.0f, goldenRatio, 0.0f }, { 1.0f, goldenRatio, 0.0f },
                { -1.0f, -goldenRatio, 0.0f }, { 1.0f, -goldenRatio, 0.0f },
                { 0.0f, -1.0f, goldenRatio }, { 0.0f, 1.0f, goldenRatio },
                { 0.0f, -1.0f, -goldenRatio }, { 0.0f, 1.0f, -goldenRatio },
                { goldenRatio, 0.0f, -1.0f }, { goldenRatio, 0.0f, 1.0f },
                { -goldenRatio, 0.0f, -1.0f }, { -goldenRatio, 0.0f, 1.0f }
            };
            std::vector<uint32_t> indices =
            {
                0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
                1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
                4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1
            };
            for (cVec3f& rPosition : positions)
            {
                rPosition = rPosition.normalized();
            }

            for (int subdivision = 0; subdivision < _subdivisions; ++subdivision)
            {
                std::unordered_map<uint64_t, uint32_t> midpoints;
                const auto midpoint = [&positions, &midpoints](uint32_t _a, uint32_t _b)
                {
                    const uint64_t key = (static_cast<uint64_t>(std::min(_a, _b)) << 32) | std::max(_a, _b);
                    const auto found = midpoints.find(key);
                    if (found != midpoints.end())
                    {
                        return found->second;
                    }

                    const uint32_t index = static_cast<uint32_t>(positions.size());
                    positions.push_back((positions[_a] + positions[_b]).normalized());
                    midpoints.emplace(key, index);
                    return index;
                };

                std::vector<uint32_t> refined;
                refined.reserve(indices.size() * 4);
                for (size_t triangle = 0; triangle < indices.size(); triangle += 3)
                {
                    const uint32_t a = indices[triangle];
                    const uint32_t b = indices[triangle + 1];
                    const uint32_t c = indices[triangle + 2];
                    const uint32_t ab = midpoint(a, b);
                    const uint32_t bc = midpoint(b, c);
                    const uint32_t ca = midpoint(c, a);
                    refined.insert(refined.end(), { a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca });
                }
                indices = std::move(refined);
            }

            sMeshData mesh{};
            mesh.indices = std::move(indices);
            for (const cVec3f& rNormal : positions)
            {
                // Fixed integer PRNG, independent of standard-library distribution implementations.
                _seed = _seed * 1664525u + 1013904223u;
                const float random = static_cast<float>(_seed >> 8) / 16777215.0f;
                const float radius = _radius * (1.0f + _roughness * (2.0f * random - 1.0f));
                const float u = 0.5f + std::atan2(rNormal.z(), rNormal.x()) / (2.0f * c_pi);
                const float v = std::acos(std::clamp(rNormal.y(), -1.0f, 1.0f)) / c_pi;
                mesh.vertices.push_back({ .position = rNormal * radius, .normal = rNormal, .uv = { u, v } });
            }

            // Share sphere vertices except where longitude wraps or converges at a pole.
            std::unordered_map<uint32_t, uint32_t> seamVertices;
            for (size_t triangle = 0; triangle < mesh.indices.size(); triangle += 3)
            {
                std::array<uint32_t, 3> face{ mesh.indices[triangle], mesh.indices[triangle + 1], mesh.indices[triangle + 2] };
                float minU = 1.0f;
                float maxU = 0.0f;
                for (uint32_t index : face)
                {
                    if (std::abs(mesh.vertices[index].normal.y()) < 0.999999f)
                    {
                        minU = std::min(minU, mesh.vertices[index].uv[0]);
                        maxU = std::max(maxU, mesh.vertices[index].uv[0]);
                    }
                }
                for (uint32_t& rIndex : face)
                {
                    if (maxU - minU > 0.5f && mesh.vertices[rIndex].uv[0] <= 0.5f)
                    {
                        const auto found = seamVertices.find(rIndex);
                        if (found != seamVertices.end())
                        {
                            rIndex = found->second;
                        }
                        else
                        {
                            sVertex vertex = mesh.vertices[rIndex];
                            vertex.uv[0] += 1.0f;
                            const uint32_t index = static_cast<uint32_t>(mesh.vertices.size());
                            seamVertices.emplace(rIndex, index);
                            mesh.vertices.push_back(vertex);
                            rIndex = index;
                        }
                    }
                }
                for (size_t corner = 0; corner < face.size(); ++corner)
                {
                    if (std::abs(mesh.vertices[face[corner]].normal.y()) >= 0.999999f)
                    {
                        sVertex vertex = mesh.vertices[face[corner]];
                        vertex.uv[0] = (mesh.vertices[face[(corner + 1) % 3]].uv[0] + mesh.vertices[face[(corner + 2) % 3]].uv[0]) * 0.5f;
                        face[corner] = static_cast<uint32_t>(mesh.vertices.size());
                        mesh.vertices.push_back(vertex);
                    }
                    mesh.indices[triangle + corner] = face[corner];
                }
            }
            return mesh;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        double Cross2(const Math::cVec2f& _rA, const Math::cVec2f& _rB, const Math::cVec2f& _rC)
        {
            return (static_cast<double>(_rB.x()) - _rA.x()) * (static_cast<double>(_rC.y()) - _rA.y())
                - (static_cast<double>(_rB.y()) - _rA.y()) * (static_cast<double>(_rC.x()) - _rA.x());
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool OnSegment(const Math::cVec2f& _rA, const Math::cVec2f& _rB, const Math::cVec2f& _rPoint)
        {
            return Cross2(_rA, _rB, _rPoint) == 0.0
                && _rPoint.x() >= std::min(_rA.x(), _rB.x()) && _rPoint.x() <= std::max(_rA.x(), _rB.x())
                && _rPoint.y() >= std::min(_rA.y(), _rB.y()) && _rPoint.y() <= std::max(_rA.y(), _rB.y());
        }
    }

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

    sMeshData cMeshGenerator::CreateTorus(const sTorusDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedTorus";

        const float tubeRadius      = std::max(_rDesc.tubeRadius, 0.001f);
        const float majorRadius     = std::max(_rDesc.majorRadius, tubeRadius + 0.001f);
        const int   segments        = std::clamp(_rDesc.segments, 3, 256);
        const int   tubeSegments    = std::clamp(_rDesc.tubeSegments, 3, 128);
        const int   stride          = tubeSegments + 1;

        mesh.vertices.reserve((segments + 1) * stride);
        mesh.indices.reserve(segments * tubeSegments * 6);

        // The ring lies in XZ, with its opening along Y.
        for (int segment = 0; segment <= segments; ++segment)
        {
            const float u       = static_cast<float>(segment) / segments;
            const float theta   = 2.0f * c_pi * u;
            const cVec3f radial{ std::sin(theta), 0.0f, std::cos(theta) };

            for (int tubeSegment = 0; tubeSegment <= tubeSegments; ++tubeSegment)
            {
                const float v       = static_cast<float>(tubeSegment) / tubeSegments;
                const float phi     = 2.0f * c_pi * v;
                const cVec3f normal = radial * std::cos(phi) + cVec3f{ 0.0f, std::sin(phi), 0.0f };

                mesh.vertices.push_back({ .position = radial * majorRadius + normal * tubeRadius, .normal = normal, .uv = { u, v } });
            }
        }

        for (int segment = 0; segment < segments; ++segment)
        {
            for (int tubeSegment = 0; tubeSegment < tubeSegments; ++tubeSegment)
            {
                const uint32_t a = static_cast<uint32_t>(segment * stride + tubeSegment);
                const uint32_t b = a + static_cast<uint32_t>(stride);

                mesh.indices.insert(mesh.indices.end(), { a, b, b + 1, a, b + 1, a + 1 });
            }
        }

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCrystal(const sCrystalDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCrystal";

        const float radius      = std::max(_rDesc.radius, 0.001f);
        const float height      = std::max(_rDesc.height, 0.001f);
        const int   segments    = std::clamp(_rDesc.segments, 3, 128);

        mesh.vertices.reserve(segments * 12);
        mesh.indices.reserve(segments * 12);

        const auto addTriangle = [&mesh](const cVec3f& _rA, const cVec3f& _rB, const cVec3f& _rC)
        {
            const cVec3f normal = (_rB - _rA).cross(_rC - _rA).normalized();
            const uint32_t first = static_cast<uint32_t>(mesh.vertices.size());

            mesh.vertices.push_back({ .position = _rA, .normal = normal, .uv = { 0.0f, 0.0f } });
            mesh.vertices.push_back({ .position = _rB, .normal = normal, .uv = { 1.0f, 0.0f } });
            mesh.vertices.push_back({ .position = _rC, .normal = normal, .uv = { 0.5f, 1.0f } });

            mesh.indices.insert(mesh.indices.end(), { first, first + 1, first + 2 });
        };

        // Separate face vertices keep the hexagonal shaft and pointed ends faceted.
        for (int segment = 0; segment < segments; ++segment)
        {
            const float angle       = 2.0f * c_pi * static_cast<float>(segment)     / segments;
            const float nextAngle   = 2.0f * c_pi * static_cast<float>(segment + 1) / segments;

            const cVec3f lower{ std::sin(angle) * radius, -height * 0.25f, std::cos(angle) * radius };
            const cVec3f nextLower{ std::sin(nextAngle) * radius, -height * 0.25f, std::cos(nextAngle) * radius };
            const cVec3f upper{ lower.x(), height * 0.2f, lower.z() };
            const cVec3f nextUpper{ nextLower.x(), height * 0.2f, nextLower.z() };

            addTriangle(lower, nextLower, nextUpper);
            addTriangle(lower, nextUpper, upper);
            addTriangle(upper, nextUpper, cVec3f{ 0.0f, height * 0.5f, 0.0f });
            addTriangle(nextLower, lower, cVec3f{ 0.0f, -height * 0.5f, 0.0f });
        }

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateBeveledCube(const sBeveledCubeDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.width) && IsPositive(_rDesc.height) && IsPositive(_rDesc.depth));
        Require(IsPositive(_rDesc.bevel) && _rDesc.bevel < std::min({ _rDesc.width, _rDesc.height, _rDesc.depth }) * 0.5f);

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedBeveledCube";
        const std::array<float, 3> half{ _rDesc.width * 0.5f, _rDesc.height * 0.5f, _rDesc.depth * 0.5f };
        const std::array<cVec3f, 3> axes{ cVec3f{ 1.0f, 0.0f, 0.0f }, cVec3f{ 0.0f, 1.0f, 0.0f }, cVec3f{ 0.0f, 0.0f, 1.0f } };
        mesh.vertices.reserve(96);
        mesh.indices.reserve(132);

        for (int axis = 0; axis < 3; ++axis)
        {
            const int u = (axis + 1) % 3;
            const int v = (axis + 2) % 3;
            const cVec3f du = axes[u] * (half[u] - _rDesc.bevel);
            const cVec3f dv = axes[v] * (half[v] - _rDesc.bevel);
            for (float sign : { -1.0f, 1.0f })
            {
                const cVec3f center = axes[axis] * (sign * half[axis]);
                AddFace(mesh, { center - du - dv, center + du - dv, center + du + dv, center - du + dv }, axes[axis] * sign);
            }

            // Four chamfer strips parallel to this axis.
            for (float signU : { -1.0f, 1.0f })
            {
                for (float signV : { -1.0f, 1.0f })
                {
                    const cVec3f a = axes[u] * (signU * half[u]) + axes[v] * (signV * (half[v] - _rDesc.bevel));
                    const cVec3f b = axes[u] * (signU * (half[u] - _rDesc.bevel)) + axes[v] * (signV * half[v]);
                    const cVec3f offset = axes[axis] * (half[axis] - _rDesc.bevel);
                    AddFace(mesh, { a - offset, b - offset, b + offset, a + offset }, axes[u] * signU + axes[v] * signV);
                }
            }
        }

        for (float x : { -1.0f, 1.0f })
        {
            for (float y : { -1.0f, 1.0f })
            {
                for (float z : { -1.0f, 1.0f })
                {
                    AddFace(mesh,
                        {
                            { x * half[0], y * (half[1] - _rDesc.bevel), z * (half[2] - _rDesc.bevel) },
                            { x * (half[0] - _rDesc.bevel), y * half[1], z * (half[2] - _rDesc.bevel) },
                            { x * (half[0] - _rDesc.bevel), y * (half[1] - _rDesc.bevel), z * half[2] }
                        }, { x, y, z });
                }
            }
        }

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateFrustum(const sFrustumDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.height));
        Require(std::isfinite(_rDesc.bottomRadius) && _rDesc.bottomRadius >= 0.0f);
        Require(std::isfinite(_rDesc.topRadius) && _rDesc.topRadius >= 0.0f);
        Require(_rDesc.bottomRadius > 0.0f || _rDesc.topRadius > 0.0f);
        ValidateSegments(_rDesc.segments);

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedFrustum";
        const float halfHeight = _rDesc.height * 0.5f;
        const cVec3f slope = cVec3f{ _rDesc.height, _rDesc.bottomRadius - _rDesc.topRadius, 0.0f }.normalized();
        AddLathe(mesh,
            {
                { _rDesc.bottomRadius, -halfHeight, slope.x(), slope.y() },
                { _rDesc.topRadius, halfHeight, slope.x(), slope.y() }
            }, _rDesc.segments);
        AddCap(mesh, _rDesc.bottomRadius, -halfHeight, _rDesc.segments, false);
        AddCap(mesh, _rDesc.topRadius, halfHeight, _rDesc.segments, true);

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateWedge(const sWedgeDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.width) && IsPositive(_rDesc.height) && IsPositive(_rDesc.depth));
        const float x = _rDesc.width * 0.5f;
        const float y = _rDesc.height * 0.5f;
        sMeshData mesh = CreateExtrudedPolygon({ .points = { { -x, -y }, { x, -y }, { x, y } }, .depth = _rDesc.depth });
        mesh.pDebugName = "GeneratedWedge";
        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateTriangularPrism(const sTriangularPrismDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.width) && IsPositive(_rDesc.height) && IsPositive(_rDesc.depth));
        const float x = _rDesc.width * 0.5f;
        const float y = _rDesc.height * 0.5f;
        sMeshData mesh = CreateExtrudedPolygon({ .points = { { -x, -y }, { x, -y }, { 0.0f, y } }, .depth = _rDesc.depth });
        mesh.pDebugName = "GeneratedTriangularPrism";
        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateIcoSphere(const sIcoSphereDesc& _rDesc)
    {
        sMeshData mesh = CreateGeodesic(_rDesc.radius, _rDesc.subdivisions, 0.0f, 0);
        mesh.pDebugName = "GeneratedIcoSphere";
        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateRock(const sRockDesc& _rDesc)
    {
        const sMeshData source = CreateGeodesic(_rDesc.radius, _rDesc.subdivisions, _rDesc.roughness, _rDesc.seed);
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedRock";
        mesh.vertices.reserve(source.indices.size());
        mesh.indices.reserve(source.indices.size());

        // Flat normals retain the rock's facets after radial displacement.
        for (size_t triangle = 0; triangle < source.indices.size(); triangle += 3)
        {
            const sVertex& rA = source.vertices[source.indices[triangle]];
            const sVertex& rB = source.vertices[source.indices[triangle + 1]];
            const sVertex& rC = source.vertices[source.indices[triangle + 2]];
            const cVec3f normal = (rB.position - rA.position).cross(rC.position - rA.position).normalized();
            for (size_t corner = 0; corner < 3; ++corner)
            {
                sVertex vertex = source.vertices[source.indices[triangle + corner]];
                vertex.normal = normal;
                mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size()));
                mesh.vertices.push_back(vertex);
            }
        }

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateGrassBlade(const sGrassBladeDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.width) && IsPositive(_rDesc.height) && std::isfinite(_rDesc.bend));
        ValidateSegments(_rDesc.segments, 1);

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedGrassBlade";
        for (int segment = 0; segment <= _rDesc.segments; ++segment)
        {
            const float v = static_cast<float>(segment) / _rDesc.segments;
            const float halfWidth = _rDesc.width * (1.0f - v) * 0.5f;
            const cVec3f normal = cVec3f{ 0.0f, -2.0f * _rDesc.bend * v, _rDesc.height }.normalized();
            for (float sign : { -1.0f, 1.0f })
            {
                mesh.vertices.push_back({
                    .position = { sign * halfWidth, _rDesc.height * v, _rDesc.bend * v * v },
                    .normal = normal,
                    .uv = { (sign + 1.0f) * 0.5f, v }
                });
            }
        }
        for (int segment = 0; segment < _rDesc.segments; ++segment)
        {
            const uint32_t a = static_cast<uint32_t>(segment * 2);
            mesh.indices.insert(mesh.indices.end(), { a, a + 1, a + 2 });
            if (segment + 1 < _rDesc.segments)
            {
                mesh.indices.insert(mesh.indices.end(), { a + 1, a + 3, a + 2 });
            }
        }

        if (_rDesc.doubleSided)
        {
            const uint32_t count = static_cast<uint32_t>(mesh.vertices.size());
            const size_t indexCount = mesh.indices.size();
            for (uint32_t index = 0; index < count; ++index)
            {
                sVertex vertex = mesh.vertices[index];
                vertex.normal = -vertex.normal;
                mesh.vertices.push_back(vertex);
            }
            for (size_t index = 0; index < indexCount; index += 3)
            {
                const uint32_t a = mesh.indices[index] + count;
                const uint32_t b = mesh.indices[index + 1] + count;
                const uint32_t c = mesh.indices[index + 2] + count;
                mesh.indices.insert(mesh.indices.end(), { a, c, b });
            }
        }

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCapsule(const sCapsuleDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.radius) && IsPositive(_rDesc.height) && _rDesc.height >= 2.0f * _rDesc.radius);
        ValidateSegments(_rDesc.segments);
        Require(_rDesc.rings >= 1 && _rDesc.rings <= 256);

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCapsule";
        const float offset = _rDesc.height * 0.5f - _rDesc.radius;
        std::vector<sProfilePoint> profile;
        for (int hemisphere = 0; hemisphere < 2; ++hemisphere)
        {
            for (int ring = 0; ring <= _rDesc.rings; ++ring)
            {
                if (hemisphere == 1 && ring == 0 && offset == 0.0f)
                {
                    continue;
                }

                const float angle = (static_cast<float>(ring) / _rDesc.rings + hemisphere - 1.0f) * c_pi * 0.5f;
                const bool pole = (hemisphere == 0 && ring == 0) || (hemisphere == 1 && ring == _rDesc.rings);
                const float radial = pole ? 0.0f : std::cos(angle);
                const float y = std::sin(angle);
                profile.push_back({ radial * _rDesc.radius, y * _rDesc.radius + (hemisphere == 0 ? -offset : offset), radial, y });
            }
        }
        AddLathe(mesh, profile, _rDesc.segments);

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateArch(const sArchDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.innerRadius) && IsPositive(_rDesc.outerRadius) && _rDesc.outerRadius > _rDesc.innerRadius);
        Require(IsPositive(_rDesc.depth) && std::isfinite(_rDesc.startAngle) && std::isfinite(_rDesc.endAngle));
        ValidateSegments(_rDesc.segments, 1);
        const float sweep = _rDesc.endAngle - _rDesc.startAngle;
        Require(IsPositive(sweep) && sweep < 2.0f * c_pi && sweep / _rDesc.segments < c_pi);

        sExtrudedPolygonDesc polygon{};
        polygon.points.clear();
        polygon.depth = _rDesc.depth;
        const float start = std::remainder(_rDesc.startAngle, 2.0f * c_pi);
        for (int segment = 0; segment <= _rDesc.segments; ++segment)
        {
            const float angle = start + sweep * static_cast<float>(segment) / _rDesc.segments;
            polygon.points.push_back({ std::cos(angle) * _rDesc.outerRadius, std::sin(angle) * _rDesc.outerRadius });
        }
        for (int segment = _rDesc.segments; segment >= 0; --segment)
        {
            const float angle = start + sweep * static_cast<float>(segment) / _rDesc.segments;
            polygon.points.push_back({ std::cos(angle) * _rDesc.innerRadius, std::sin(angle) * _rDesc.innerRadius });
        }

        sMeshData mesh = CreateExtrudedPolygon(polygon);
        mesh.pDebugName = "GeneratedArch";
        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateExtrudedPolygon(const sExtrudedPolygonDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.depth) && _rDesc.points.size() >= 3);
        Require(_rDesc.points.size() <= std::numeric_limits<uint32_t>::max() / 6);
        std::vector<Math::cVec2f> points = _rDesc.points;
        for (const Math::cVec2f& rPoint : points)
        {
            Require(std::isfinite(rPoint.x()) && std::isfinite(rPoint.y()));
        }
        if (points.front().x() == points.back().x() && points.front().y() == points.back().y())
        {
            points.pop_back();
        }
        Require(points.size() >= 3);

        // Reject repeated vertices, crossings, touching edges and overlapping adjacent edges.
        for (size_t i = 0; i < points.size(); ++i)
        {
            const size_t nextI = (i + 1) % points.size();
            const size_t previous = (i + points.size() - 1) % points.size();
            if (Cross2(points[previous], points[i], points[nextI]) == 0.0)
            {
                Require(OnSegment(points[previous], points[nextI], points[i]));
            }
            for (size_t j = i + 1; j < points.size(); ++j)
            {
                Require(points[i].x() != points[j].x() || points[i].y() != points[j].y());
                const size_t nextJ = (j + 1) % points.size();
                if (nextI == j || nextJ == i)
                {
                    continue;
                }

                const double a = Cross2(points[i], points[nextI], points[j]);
                const double b = Cross2(points[i], points[nextI], points[nextJ]);
                const double c = Cross2(points[j], points[nextJ], points[i]);
                const double d = Cross2(points[j], points[nextJ], points[nextI]);
                const bool crossing = ((a > 0.0 && b < 0.0) || (a < 0.0 && b > 0.0))
                    && ((c > 0.0 && d < 0.0) || (c < 0.0 && d > 0.0));
                Require(!crossing && !OnSegment(points[i], points[nextI], points[j])
                    && !OnSegment(points[i], points[nextI], points[nextJ])
                    && !OnSegment(points[j], points[nextJ], points[i])
                    && !OnSegment(points[j], points[nextJ], points[nextI]));
            }
        }

        for (size_t i = 0; points.size() > 3 && i < points.size();)
        {
            const size_t previous = (i + points.size() - 1) % points.size();
            const size_t next = (i + 1) % points.size();
            if (Cross2(points[previous], points[i], points[next]) == 0.0)
            {
                points.erase(points.begin() + i);
                i = 0;
            }
            else
            {
                ++i;
            }
        }

        double area = 0.0;
        for (size_t i = 1; i + 1 < points.size(); ++i)
        {
            area += Cross2(points[0], points[i], points[i + 1]);
        }
        Require(area != 0.0);
        if (area < 0.0)
        {
            std::reverse(points.begin(), points.end());
        }

        std::vector<uint32_t> boundary(points.size());
        std::iota(boundary.begin(), boundary.end(), 0u);
        std::vector<uint32_t> triangles;
        while (boundary.size() > 3)
        {
            bool clipped = false;
            for (size_t i = 0; i < boundary.size(); ++i)
            {
                const uint32_t a = boundary[(i + boundary.size() - 1) % boundary.size()];
                const uint32_t b = boundary[i];
                const uint32_t c = boundary[(i + 1) % boundary.size()];
                if (Cross2(points[a], points[b], points[c]) <= 0.0)
                {
                    continue;
                }

                bool containsPoint = false;
                for (uint32_t index : boundary)
                {
                    if (index != a && index != b && index != c
                        && Cross2(points[a], points[b], points[index]) >= 0.0
                        && Cross2(points[b], points[c], points[index]) >= 0.0
                        && Cross2(points[c], points[a], points[index]) >= 0.0)
                    {
                        containsPoint = true;
                        break;
                    }
                }
                if (!containsPoint)
                {
                    triangles.insert(triangles.end(), { a, b, c });
                    boundary.erase(boundary.begin() + i);
                    clipped = true;
                    break;
                }
            }
            Require(clipped);
        }
        Require(Cross2(points[boundary[0]], points[boundary[1]], points[boundary[2]]) > 0.0);
        triangles.insert(triangles.end(), boundary.begin(), boundary.end());

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedExtrudedPolygon";
        const uint32_t count = static_cast<uint32_t>(points.size());
        mesh.vertices.reserve(static_cast<size_t>(count) * 6);
        mesh.indices.reserve(static_cast<size_t>(count) * 12 - 12);

        const float halfDepth = _rDesc.depth * 0.5f;
        float minX = points[0].x();
        float minY = points[0].y();
        float maxX = minX;
        float maxY = minY;
        for (const Math::cVec2f& rPoint : points)
        {
            minX = std::min(minX, rPoint.x());
            minY = std::min(minY, rPoint.y());
            maxX = std::max(maxX, rPoint.x());
            maxY = std::max(maxY, rPoint.y());
        }
        for (float sign : { 1.0f, -1.0f })
        {
            for (const Math::cVec2f& rPoint : points)
            {
                mesh.vertices.push_back({
                    .position = { rPoint.x(), rPoint.y(), halfDepth * sign },
                    .normal = { 0.0f, 0.0f, sign },
                    .uv = { (rPoint.x() - minX) / (maxX - minX), (rPoint.y() - minY) / (maxY - minY) }
                });
            }
        }
        for (size_t triangle = 0; triangle < triangles.size(); triangle += 3)
        {
            const uint32_t a = triangles[triangle];
            const uint32_t b = triangles[triangle + 1];
            const uint32_t c = triangles[triangle + 2];
            mesh.indices.insert(mesh.indices.end(), { a, b, c, a + count, c + count, b + count });
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            const Math::cVec2f& rA = points[i];
            const Math::cVec2f& rB = points[(i + 1) % count];
            AddFace(mesh,
                {
                    { rA.x(), rA.y(), -halfDepth }, { rB.x(), rB.y(), -halfDepth },
                    { rB.x(), rB.y(), halfDepth }, { rA.x(), rA.y(), halfDepth }
                }, { rB.y() - rA.y(), rA.x() - rB.x(), 0.0f });
        }

        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateDisc(const sDiscDesc& _rDesc)
    {
        Require(IsPositive(_rDesc.radius));
        ValidateSegments(_rDesc.segments);

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedDisc";
        AddCap(mesh, _rDesc.radius, 0.0f, _rDesc.segments, true);
        mesh.bounds = CalculateBounds(mesh.vertices);
        return mesh;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateArc(const sArcDesc& _rDesc)
    {
        Require(std::isfinite(_rDesc.innerRadius) && _rDesc.innerRadius >= 0.0f);
        Require(IsPositive(_rDesc.outerRadius) && _rDesc.outerRadius > _rDesc.innerRadius);
        Require(std::isfinite(_rDesc.startAngle) && std::isfinite(_rDesc.endAngle));
        ValidateSegments(_rDesc.segments, 1);
        const float sweep = _rDesc.endAngle - _rDesc.startAngle;
        Require(IsPositive(sweep) && sweep <= 2.0f * c_pi && sweep / _rDesc.segments < c_pi);

        sMeshData mesh{};
        mesh.pDebugName = "GeneratedArc";
        const float start = std::remainder(_rDesc.startAngle, 2.0f * c_pi);
        const bool fullRing = sweep == 2.0f * c_pi;
        const bool sector = _rDesc.innerRadius == 0.0f;
        const int count = fullRing ? _rDesc.segments : _rDesc.segments + 1;
        if (sector)
        {
            mesh.vertices.push_back({ .position = { 0.0f, 0.0f, 0.0f }, .normal = { 0.0f, 1.0f, 0.0f }, .uv = { 0.5f, 0.5f } });
        }
        for (int segment = 0; segment < count; ++segment)
        {
            const float angle = start + sweep * static_cast<float>(segment) / _rDesc.segments;
            for (float radius : { _rDesc.innerRadius, _rDesc.outerRadius })
            {
                if (sector && radius == 0.0f)
                {
                    continue;
                }
                const float x = std::sin(angle) * radius;
                const float z = std::cos(angle) * radius;
                mesh.vertices.push_back({
                    .position = { x, 0.0f, z }, .normal = { 0.0f, 1.0f, 0.0f },
                    .uv = { 0.5f + x / _rDesc.outerRadius * 0.5f, 0.5f + z / _rDesc.outerRadius * 0.5f }
                });
            }
        }
        for (int segment = 0; segment < _rDesc.segments; ++segment)
        {
            const uint32_t next = static_cast<uint32_t>((segment + 1) % count);
            if (sector)
            {
                mesh.indices.insert(mesh.indices.end(), { 0, static_cast<uint32_t>(segment + 1), next + 1 });
            }
            else
            {
                const uint32_t a = static_cast<uint32_t>(segment * 2);
                const uint32_t b = next * 2;
                mesh.indices.insert(mesh.indices.end(), { a, a + 1, b + 1, a, b + 1, b });
            }
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
