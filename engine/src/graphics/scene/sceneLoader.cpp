#include "sceneLoader.h"

#include "scene.h"

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

        GFX::sTransform ParseTransform(const nlohmann::json& _rJson)
        {
            GFX::sTransform transform{};

            transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
            transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
            transform.scale    = Math::cVec3f(1.0f, 1.0f, 1.0f);

            if (_rJson.contains("position"))
                transform.position = ParseVec3(_rJson.at("position"));

            if (_rJson.contains("rotation"))
                transform.rotation = ParseVec3(_rJson.at("rotation"));

            if (_rJson.contains("scale"))
                transform.scale    = ParseVec3(_rJson.at("scale"));

            return transform;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool SceneLoader::LoadFromFile(const std::filesystem::path& _rFilePath, cScene& _rScene, std::string& _rErrorMessage)
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

            std::unordered_map<std::string, GFX::ShapeModelHandle> modelHandles;

            // -------------------------------------------------------------------------------------------------------------------------
            // Models
            // -------------------------------------------------------------------------------------------------------------------------

            for (const nlohmann::json& modelJson : sceneJson.at("models"))
            {
                const std::string modelId = modelJson.at("id").get<std::string>();
                const std::filesystem::path relativeModelPath = modelJson.at("file").get<std::string>();

                if (modelId.empty())
                {
                    _rErrorMessage = "Scene contains a model with an empty id.";
                    return false;
                }

                if (modelHandles.find(modelId) != modelHandles.end())
                {
                    _rErrorMessage = "Duplicate model id in scene: " + modelId;
                    return false;
                }

                const std::filesystem::path modelPath = (_rFilePath.parent_path() / relativeModelPath).lexically_normal();

                GFX::sShapeModelDesc modelDesc;
                std::string modelErrorMessage;

                if (!GFX::ShapeModelLoader::LoadFromFile(modelPath, modelDesc, modelErrorMessage))
                {
                    _rErrorMessage = "Failed to load scene model '" + modelId + "': " + modelErrorMessage;
                    return false;
                }

                const GFX::ShapeModelHandle modelHandle = GFX::ShapeModelManager::CreateShapeModel(modelDesc);

                modelHandles.emplace(modelId, modelHandle);
            }

            // -------------------------------------------------------------------------------------------------------------------------
            // Instances
            // -------------------------------------------------------------------------------------------------------------------------

            cScene loadedScene;

            for (const nlohmann::json& instanceJson : sceneJson.at("instances"))
            {
                const std::string modelId = instanceJson.at("model").get<std::string>();

                const auto modelIterator = modelHandles.find(modelId);

                if (modelIterator == modelHandles.end())
                {
                    _rErrorMessage = "Scene instance references unknown model: " + modelId;
                    return false;
                }

                GFX::sShapeInstance shapeInstance{};

                shapeInstance.modelHandle = modelIterator->second;
                shapeInstance.transform = ParseTransform(instanceJson);

                if (instanceJson.contains("name"))
                {
                    const std::string instanceName = instanceJson.at("name").get<std::string>();

                    if (loadedScene.AddNamedShapeInstance(instanceName, shapeInstance) == c_invalidSceneShapeInstanceHandle)
                    {
                        _rErrorMessage = "Duplicate or invalid scene instance name: " + instanceName;
                        return false;
                    }
                }
                else
                {
                    loadedScene.AddShapeInstance(shapeInstance);
                }
            }

            _rScene = std::move(loadedScene);

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

}

// -------------------------------------------------------------------------------------------------------------------------