#pragma once

#include "graphics/shapeModel/shapePartDesc.h"
#include "graphics/bounds.h"

#include <string>
#include <vector>

namespace Engine::GFX
{
    struct sShapeModelDesc
    {
        std::string pDebugName; 
        std::vector<sShapePartDesc> shapes;

        std::vector<uint32_t> materialIndices;

        sBounds bounds;
    };
}