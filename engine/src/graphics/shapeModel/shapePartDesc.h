#pragma once 

#include "graphics/transform.h"

#include "graphics/shapeModel/meshType.h"

#include <string>

namespace Engine::GFX
{
    struct sShapePartDesc
    {
        std::string         name;
        sMeshTypes::Enum    meshType; 
        sTransform          transform; 
        float               color[4];
    };
}