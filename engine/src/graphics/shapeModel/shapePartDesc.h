#pragma once 

#include "graphics/transform.h"

#include "graphics/shapeModel/meshType.h"

namespace Engine::GFX
{
    struct sShapePartDesc
    {
        sMeshTypes::Enum    meshType; 
        sTransform          transform; 
        float               color[4];
    };
}