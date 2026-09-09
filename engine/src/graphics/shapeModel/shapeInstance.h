#pragma once 

#include "graphics/transform.h"

#include "graphics/shapeModel/shapeModelManager.h"

namespace Engine::GFX
{
    enum class eShapeCollisionMode
    {
        BiomeProxies,
        Disabled,
        Mesh
    };

    struct sShapeInstance
    {
        ShapeModelHandle    modelHandle;
        sTransform          transform;
        bool                generateLights = true;
        eShapeCollisionMode collisionMode = eShapeCollisionMode::BiomeProxies;
    };
}
