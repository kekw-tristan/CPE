#pragma once

#include "graphics/shapeModel/shapePartDesc.h"

#include <string>
#include <vector>

namespace Engine::GFX
{
    struct sShapeModelDesc
    {
        std::string pDebugName; 
        std::vector<sShapePartDesc> shapes;
    };
}