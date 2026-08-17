#pragma once
#pragma once

#include <filesystem>
#include <string>

namespace Engine::GFX
{
    class cScene;

    namespace SceneLoader
    {
        bool LoadFromFile(const std::filesystem::path& _rFilePath, cScene& _rScene, std::string& _rErrorMessage);
    }
}