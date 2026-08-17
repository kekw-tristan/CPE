#pragma once

#include "graphics/transform.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Engine::GFX
{
    struct sSceneModelDesc
    {
        std::string id;
        std::filesystem::path filePath;
    };

    struct sSceneShapeInstanceDesc
    {
        std::string name;
        std::string modelId;
        GFX::sTransform transform;
    };

    struct sSceneDesc
    {
        std::string name;
        std::vector<sSceneModelDesc> models;
        std::vector<sSceneShapeInstanceDesc> shapeInstances;
    };
}
