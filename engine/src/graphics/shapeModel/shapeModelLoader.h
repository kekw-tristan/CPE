#pragma once

#include <filesystem>
#include <string>

namespace Engine::GFX
{
    struct sShapeModelDesc;

    namespace ShapeModelLoader
    {
        bool LoadFromFile(const std::filesystem::path& _rFilePath, sShapeModelDesc& _rModelDesc, std::string& _rErrorMessage);
        bool SaveToFile(const std::filesystem::path& _rFilePath, const sShapeModelDesc& _rModelDesc, std::string& _rErrorMessage);
    }
}