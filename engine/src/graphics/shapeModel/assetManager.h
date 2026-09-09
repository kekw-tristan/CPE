#pragma once

#include "graphics/shapeModel/assetHandle.h"

#include <filesystem>

namespace Engine::GFX
{
    namespace AssetManager
    {
        sAssetHandle Load(const std::filesystem::path& _rFilePath);
    }
}