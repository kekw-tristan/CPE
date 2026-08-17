#pragma once

#include <filesystem>
#include <string>

namespace Engine::GFX
{
    class cScene;
    struct sSceneDesc;

    namespace SceneLoader
    {
        bool LoadDescFromFile(const std::filesystem::path& _rFilePath, sSceneDesc& _rSceneDesc, std::string& _rErrorMessage);
        bool SaveToFile(const std::filesystem::path& _rFilePath, const sSceneDesc& _rSceneDesc, std::string& _rErrorMessage);

        bool LoadFromFile(const std::filesystem::path& _rFilePath, cScene& _rScene, std::string& _rErrorMessage);
    }
}
