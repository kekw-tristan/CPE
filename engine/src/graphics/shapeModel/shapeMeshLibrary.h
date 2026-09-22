#pragma once

#include "graphics/bounds.h"
#include "graphics/meshData.h"
#include "graphics/shapeModel/meshType.h"

#include <array>

namespace Engine::GFX
{
    struct sShapeModelDesc;

    struct sShapeTriangleBatch
    {
        sMeshData mesh{};
        std::array<float, 4> color{};
        uint32_t materialIndex = 0;
    };

    namespace ShapeMeshLibrary
    {
        sMeshData& GetMeshData(sMeshTypes::Enum _meshType);
        const sBounds& GetBounds(sMeshTypes::Enum _meshType);

        // Bake immutable triangle-only models in model space, grouped by material and tint.
        // Unsupported models return no batches and retain the ordinary shape path.
        std::vector<sShapeTriangleBatch> BakeTriangleModel(const sShapeModelDesc& _rModel);
    }
}
