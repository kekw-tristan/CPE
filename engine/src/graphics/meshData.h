#pragma once

#include "graphics/vertex.h"
#include "graphics/bounds.h"

#include <vector> 
#include <cstdint>

namespace Engine::GFX
{
    struct sMeshData
    {
        std::vector<sVertex>    vertices;
        std::vector<uint32_t>   indices;
        sBounds                 bounds;
        const char*             pDebugName; 
    };
}