#pragma once 

#include "math/vector3.h"

namespace Engine::GFX
{
    struct sTransform
    {
        Math::cVec3f position   = {}; 
        Math::cVec3f scale      = {}; 
        Math::cVec3f rotation   = {}; 
    };
}