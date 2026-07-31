#pragma once

#include "graphics/shapeModel/shapePartDesc.h"

#include <vector>

namespace Engine::GFX
{
    struct sShapeModelDesc
    {
        const char* pDebugName; 
        std::vector<sShapePartDesc> shapes;
    };
}