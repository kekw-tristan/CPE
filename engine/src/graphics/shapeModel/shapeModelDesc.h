#pragma once

#include "graphics/shapeModel/shapePartDesc.h"
#include "graphics/shapeModel/shapeLightDesc.h"
#include "graphics/bounds.h"

#include <string>
#include <vector>

namespace Engine::GFX
{
    struct sShapeModelDesc
    {
        std::string pDebugName; 
        std::vector<sShapePartDesc> shapes;
        std::vector<sShapeLightDesc> lights;

        std::vector<uint32_t> materialIndices;

        sBounds bounds;
    };
}
