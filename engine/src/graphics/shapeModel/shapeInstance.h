#pragma once 

#include "graphics/transform.h"

#include "graphics/shapeModel/shapeModelManager.h"

namespace Engine::GFX
{
    struct sShapeInstance
    {
        ShapeModelHandle    modelHandle;
        sTransform          transform;
    };
}