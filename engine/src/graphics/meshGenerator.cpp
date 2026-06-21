#include "meshGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData cMeshGenerator::CreateCube(const sCubeDesc& _rDesc)
    {
        sMeshData mesh{};
        mesh.pDebugName = "GeneratedCube";

        mesh.vertices.reserve(24);
        mesh.indices.reserve(36);

        const float hx = _rDesc.width  * 0.5f;
        const float hy = _rDesc.height * 0.5f;
        const float hz = _rDesc.depth  * 0.5f; 

        const std::array<float, 3> positions[24] =
        {
            // Front +Z
            { -hx, -hy,  hz },
            {  hx, -hy,  hz },
            {  hx,  hy,  hz },
            { -hx,  hy,  hz },

            // Back -Z
            {  hx, -hy, -hz },
            { -hx, -hy, -hz },
            { -hx,  hy, -hz },
            {  hx,  hy, -hz },

            // Left -X
            { -hx, -hy, -hz },
            { -hx, -hy,  hz },
            { -hx,  hy,  hz },
            { -hx,  hy, -hz },

            // Right +X
            { hx, -hy,  hz },
            { hx, -hy, -hz },
            { hx,  hy, -hz },
            { hx,  hy,  hz },

            // Top +Y
            { -hx, hy,  hz },
            {  hx, hy,  hz },
            {  hx, hy, -hz },
            { -hx, hy, -hz },

            // Bottom -Y
            { -hx, -hy, -hz },
            {  hx, -hy, -hz },
            {  hx, -hy,  hz },
            { -hx, -hy,  hz }
        };

        const std::array<float, 3> normals[6] =
        {
            {  0.0f,  0.0f,  1.0f }, // Front
            {  0.0f,  0.0f, -1.0f }, // Back
            { -1.0f,  0.0f,  0.0f }, // Left
            {  1.0f,  0.0f,  0.0f }, // Right
            {  0.0f,  1.0f,  0.0f }, // Top
            {  0.0f, -1.0f,  0.0f }  // Bottom
        };

        const std::array<float, 2> uvs[4] =
        {
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
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
                vertex.normal   = normals[faceIndex];
                vertex.uv       = uvs[vertexIndex];
                vertex.color    = _rDesc.color;

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

    sBounds cMeshGenerator::CalculateBounds(const std::vector<sVertex>& _rVertices)
    {
        sBounds bounds{};

        if (_rVertices.empty())
        {
            sBounds bounds = {
                {0.0f, 0.0f, 0.0f}, // min
                {0.0f, 0.0f, 0.0f}, // max
                {0.0f, 0.0f, 0.0f}, // center
                {0.0f, 0.0f, 0.0f}, // size
                0.0f                // radius
            };
            return bounds;
        }

        
        constexpr float c_floatMax = std::numeric_limits<float>::max();
        constexpr float c_floatMin = std::numeric_limits<float>::lowest();

        std::array<float, 3> minValue = { c_floatMax, c_floatMax, c_floatMax };
        std::array<float, 3> maxValue = { c_floatMin, c_floatMin, c_floatMin };

        for (const sVertex& rVertex : _rVertices)
        {
            for(int index = 0; index < 3; ++index)
            {
                minValue[index] = std::min(minValue[index], rVertex.position[index]);
                maxValue[index] = std::max(maxValue[index], rVertex.position[index]);
            }
        }

        bounds.min = minValue; 
        bounds.max = maxValue; 
        
        for (int index = 0; index < 3; ++index)
        {
            bounds.center[index] = (bounds.min[index] + bounds.max[index]) * 0.5;
            bounds.size[index]   = bounds.max[index] - bounds.min[index];
        }

        float maxRadiusSquared = 0.f; 

        for (const sVertex& rVertex : _rVertices)
        {
            float distanceSquared = 0.f;

            for (int index = 0; index < 3; ++index)
            {
                const float difference = rVertex.position[index] - bounds.center[index]; 
                distanceSquared += difference * difference;
            }

            maxRadiusSquared = std::max(maxRadiusSquared, distanceSquared);
        }

        bounds.radius = std::sqrt(maxRadiusSquared);

        return bounds;

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------