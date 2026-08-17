#include "sceneLoader.h"

#include "scene.h"
#include "sceneDesc.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

#include <nlohmann/json.hpp>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f ParseVec3(const nlohmann::json& _rJson)
        {
            return Math::cVec3f(_rJson.at(0).get<float>(), _rJson.at(1).get<float>(), _rJson.at(2).get<float>());
        }

        // -------------------------------------------------------------------------------------------------------------------------

        nlohmann::json Vec3ToJson(const Math::cVec3f& _rVector)
        {
            return nlohmann::json{ _rVector.x(), _rVector.y(), _rVector.z() };
        }

        // -------------------------------------------------------------------------------------------------------------------------

        GFX::sTransform ParseTransform(const nlohmann::json& _rJson)
        {
            GFX::sTransform transform{};

            transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
            transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
            transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

            if (_rJson.contains("position"))
                transform.position = ParseVec3(_rJson.at("position"));

            if (_rJson.contains("rotation"))
                transform.rotation = ParseVec3(_rJson.at("rotation"));

            if (_rJson.contains("scale"))
                transform.scale = ParseVec3(_rJson.at("scale"));

            return transform;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool ValidateSceneDesc(const sSceneDesc& _rSceneDesc, std::string& _rErrorMessage)
        {
            std::unordered_map<std::string, bool> modelIds;
            std::unordered_map<std::string, bool> instanceNames;

            for (const sSceneModelDesc& modelDesc : _rSceneDesc.models)
            {
                if (modelDesc.id.empty())
                {
                    _rErrorMessage = "Scene contains a model with an empty id.";
                    return false;
                }

                if (modelDesc.filePath.empty())
                {
                    _rErrorMessage = "Scene model '" + modelDesc.id + "' has an empty file path.";
                    return false;
                }

                if (modelIds.contains(modelDesc.id))
                {
                    _rErrorMessage = "Duplicate model id in scene: " + modelDesc.id;
                    return false;
                }

                modelIds.emplace(modelDesc.id, true);
            }

            for (const sSceneShapeInstanceDesc& instanceDesc : _rSceneDesc.shapeInstances)
            {
                if (!modelIds.contains(instanceDesc.modelId))
                {
                    _rErrorMessage = "Scene instance references unknown model: " + instanceDesc.modelId;
                    return false;
                }

                if (!instanceDesc.name.empty())
                {
                    if (instanceNames.contains(instanceDesc.name))
                    {
                        _rErrorMessage = "Duplicate scene instance name: " + instanceDesc.name;
                        return false;
                    }

                    instanceNames.emplace(instanceDesc.name, true);
                }
            }

            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool SceneLoader::LoadDescFromFile(const std::filesystem::path& _rFilePath, sSceneDesc& _rSceneDesc, std::string& _rErrorMessage)
    {
        std::ifstream file(_rFilePath);

        if (!file.is_open())
        {
            _rErrorMessage = "Could not open scene file: " + _rFilePath.string();
            return false;
        }

        try
        {
            nlohmann::json sceneJson;
            file >> sceneJson;

            if (!sceneJson.contains("models") || !sceneJson.contains("instances"))
            {
                _rErrorMessage = "Scene must contain models and instances.";
                return false;
            }

            sSceneDesc loadedSceneDesc;
            loadedSceneDesc.name = sceneJson.value("name", _rFilePath.stem().string());

            for (const nlohmann::json& modelJson : sceneJson.at("models"))
            {
                sSceneModelDesc modelDesc;

                modelDesc.id = modelJson.at("id").get<std::string>();
                modelDesc.filePath = modelJson.at("file").get<std::string>();

                loadedSceneDesc.models.push_back(std::move(modelDesc));
            }

            for (const nlohmann::json& instanceJson : sceneJson.at("instances"))
            {
                sSceneShapeInstanceDesc instanceDesc;

                instanceDesc.name = instanceJson.value("name", std::string{});
                instanceDesc.modelId = instanceJson.at("model").get<std::string>();
                instanceDesc.transform = ParseTransform(instanceJson);

                loadedSceneDesc.shapeInstances.push_back(std::move(instanceDesc));
            }

            if (!ValidateSceneDesc(loadedSceneDesc, _rErrorMessage))
                return false;

            _rSceneDesc = std::move(loadedSceneDesc);

            _rErrorMessage.clear();

            return true;
        }
        catch (const nlohmann::json::exception& _rException)
        {
            _rErrorMessage = "Invalid scene JSON: " + std::string(_rException.what());
            return false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool SceneLoader::SaveToFile(const std::filesystem::path& _rFilePath, const sSceneDesc& _rSceneDesc, std::string& _rErrorMessage)
    {
        if (!ValidateSceneDesc(_rSceneDesc, _rErrorMessage))
            return false;

        std::ofstream file(_rFilePath);

        if (!file.is_open())
        {
            _rErrorMessage = "Could not open scene file for writing: " + _rFilePath.string();
            return false;
        }

        try
        {
            nlohmann::json sceneJson;

            sceneJson["name"] = _rSceneDesc.name;
            sceneJson["models"] = nlohmann::json::array();
            sceneJson["instances"] = nlohmann::json::array();

            for (const sSceneModelDesc& modelDesc : _rSceneDesc.models)
            {
                nlohmann::json modelJson;

                modelJson["id"] = modelDesc.id;
                modelJson["file"] = modelDesc.filePath.generic_string();

                sceneJson["models"].push_back(std::move(modelJson));
            }

            for (const sSceneShapeInstanceDesc& instanceDesc : _rSceneDesc.shapeInstances)
            {
                nlohmann::json instanceJson;

                if (!instanceDesc.name.empty())
                    instanceJson["name"] = instanceDesc.name;

                instanceJson["model"] = instanceDesc.modelId;
                instanceJson["position"] = Vec3ToJson(instanceDesc.transform.position);
                instanceJson["rotation"] = Vec3ToJson(instanceDesc.transform.rotation);
                instanceJson["scale"] = Vec3ToJson(instanceDesc.transform.scale);

                sceneJson["instances"].push_back(std::move(instanceJson));
            }

            file << sceneJson.dump(4) << '\n';

            if (!file.good())
            {
                _rErrorMessage = "Could not write scene file: " + _rFilePath.string();
                return false;
            }

            _rErrorMessage.clear();

            return true;
        }
        catch (const nlohmann::json::exception& _rException)
        {
            _rErrorMessage = "Could not create scene JSON: " + std::string(_rException.what());
            return false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool SceneLoader::LoadFromFile(const std::filesystem::path& _rFilePath, cScene& _rScene, std::string& _rErrorMessage)
    {
        sSceneDesc sceneDesc;

        if (!LoadDescFromFile(_rFilePath, sceneDesc, _rErrorMessage))
            return false;

        std::unordered_map<std::string, GFX::ShapeModelHandle> modelHandles;

        for (const sSceneModelDesc& modelDesc : sceneDesc.models)
        {
            const std::filesystem::path modelPath = (_rFilePath.parent_path() / modelDesc.filePath).lexically_normal();

            GFX::sShapeModelDesc shapeModelDesc;
            std::string modelErrorMessage;

            if (!GFX::ShapeModelLoader::LoadFromFile(modelPath, shapeModelDesc, modelErrorMessage))
            {
                _rErrorMessage = "Failed to load scene model '" + modelDesc.id + "': " + modelErrorMessage;
                return false;
            }

            const GFX::ShapeModelHandle modelHandle = GFX::ShapeModelManager::CreateShapeModel(shapeModelDesc);
            modelHandles.emplace(modelDesc.id, modelHandle);
        }

        cScene loadedScene;

        for (const sSceneShapeInstanceDesc& instanceDesc : sceneDesc.shapeInstances)
        {
            const auto modelIterator = modelHandles.find(instanceDesc.modelId);

            if (modelIterator == modelHandles.end())
            {
                _rErrorMessage = "Scene instance references unknown model: " + instanceDesc.modelId;
                return false;
            }

            GFX::sShapeInstance shapeInstance{};
            shapeInstance.modelHandle = modelIterator->second;
            shapeInstance.transform = instanceDesc.transform;

            if (instanceDesc.name.empty())
            {
                loadedScene.AddShapeInstance(shapeInstance);
            }
            else if (loadedScene.AddNamedShapeInstance(instanceDesc.name, shapeInstance) == c_invalidSceneShapeInstanceHandle)
            {
                _rErrorMessage = "Duplicate or invalid scene instance name: " + instanceDesc.name;
                return false;
            }
        }

        _rScene = std::move(loadedScene);

        _rErrorMessage.clear();

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
