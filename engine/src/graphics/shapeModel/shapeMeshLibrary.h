#pragma once

#include "graphics/bounds.h"
#include "graphics/meshData.h"
#include "graphics/shapeModel/meshType.h"

namespace Engine::GFX
{
    namespace ShapeMeshLibrary
    {
        sMeshData& GetMeshData(sMeshTypes::Enum _meshType);
        const sBounds& GetBounds(sMeshTypes::Enum _meshType);
    }
}