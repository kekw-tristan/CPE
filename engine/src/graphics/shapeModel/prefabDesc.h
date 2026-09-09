#pragma once

#include "graphics/transform.h"
#include "graphics/shapeModel/shapeModelManager.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Engine::GFX
{

    using PrefabHandle = int32_t;

    struct sPrefabObjectDesc
    {
        ShapeModelHandle modelHandle = -1;

        sTransform transform{};

        bool generateColliders = false;
        bool generateLights = true;
    };


    struct sPrefabDesc
    {
        std::string name;

        std::vector<sPrefabObjectDesc> objects;
    };
}
