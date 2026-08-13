#pragma once 

#include "graphics/transform.h"

#include "graphics/shapeModel/meshType.h"

#include <string>
#include <cstdint>

namespace Engine::GFX
{
    struct sShapePartDesc
    {
        std::string         name;
        sMeshTypes::Enum    meshType; 
        sTransform          transform; 
        float               color[4];
        uint32_t            materialIndex;
    };
}