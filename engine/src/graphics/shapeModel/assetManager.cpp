#include "assetManager.h"

#include "graphics/shapeModel/prefabDesc.h"
#include "graphics/shapeModel/prefabManager.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

        class cAssetManager
        {

        public:

            static cAssetManager& GetInstance();


        public:

            sAssetHandle Load(const std::filesystem::path& _rFilePath);


        private:

            sAssetHandle LoadShapeModel(const std::filesystem::path& _rFilePath);
            sAssetHandle LoadPrefab(const std::filesystem::path& _rFilePath, const nlohmann::json& _rPrefabJson);


        private:

            cAssetManager();
            ~cAssetManager();

            cAssetManager(const cAssetManager&) = delete;
            cAssetManager& operator=(const cAssetManager&) = delete;

            cAssetManager(cAssetManager&&) = delete;
            cAssetManager& operator=(cAssetManager&&) = delete;


        private:

            std::unordered_map<std::string, sAssetHandle> m_loadedAssets;

        };

        // -------------------------------------------------------------------------------------------------------------------------

        cAssetManager& cAssetManager::GetInstance()
        {
            static cAssetManager s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sAssetHandle cAssetManager::Load(const std::filesystem::path& _rFilePath)
        {
            const std::filesystem::path normalizedFilePath = _rFilePath.lexically_normal();
            const std::string filePath = normalizedFilePath.generic_string();

            const auto loadedAssetIterator = m_loadedAssets.find(filePath);

            if (loadedAssetIterator != m_loadedAssets.end())
                return loadedAssetIterator->second;

            std::ifstream file(normalizedFilePath);

            if (!file.is_open())
                throw std::runtime_error("Could not open asset file: " + filePath);

            nlohmann::json assetJson;

            try
            {
                file >> assetJson;
            }
            catch (const nlohmann::json::exception& exception)
            {
                throw std::runtime_error("Invalid asset JSON '" + filePath + "': " + exception.what());
            }

            std::string assetType = assetJson.value("assetType", std::string());

            if (assetType.empty() && assetJson.contains("shapes"))
                assetType = "ShapeModel";

            sAssetHandle assetHandle{};

            if (assetType == "ShapeModel")
                assetHandle = LoadShapeModel(normalizedFilePath);
            else if (assetType == "Prefab")
                assetHandle = LoadPrefab(normalizedFilePath, assetJson);
            else
                throw std::runtime_error("Unknown asset type '" + assetType + "' in asset: " + filePath);

            if (!assetHandle.IsValid())
                throw std::runtime_error("Failed to create valid asset handle for: " + filePath);

            m_loadedAssets.emplace(filePath, assetHandle);

            return assetHandle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sAssetHandle cAssetManager::LoadShapeModel(const std::filesystem::path& _rFilePath)
        {
            sShapeModelDesc modelDesc{};
            std::string errorMessage;

            if (!ShapeModelLoader::LoadFromFile(_rFilePath, modelDesc, errorMessage))
                throw std::runtime_error("Failed to load ShapeModel '" + _rFilePath.string() + "': " + errorMessage);

            const ShapeModelHandle modelHandle = ShapeModelManager::CreateShapeModel(modelDesc);

            if (modelHandle < 0)
                throw std::runtime_error("Failed to create ShapeModel: " + _rFilePath.string());

            sAssetHandle assetHandle{};

            assetHandle.type = sAssetType::ShapeModel;
            assetHandle.handle = modelHandle;

            return assetHandle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sAssetHandle cAssetManager::LoadPrefab(const std::filesystem::path& _rFilePath, const nlohmann::json& _rPrefabJson)
        {
            sPrefabDesc prefabDesc{};

            prefabDesc.name = _rPrefabJson.value("name", _rFilePath.stem().string());

            if (!_rPrefabJson.contains("objects"))
                throw std::runtime_error("Prefab does not contain an 'objects' array: " + _rFilePath.string());

            const nlohmann::json& objectsJson = _rPrefabJson.at("objects");

            if (!objectsJson.is_array())
                throw std::runtime_error("Prefab 'objects' is not an array: " + _rFilePath.string());

            prefabDesc.objects.reserve(objectsJson.size());

            for (const nlohmann::json& objectJson : objectsJson)
            {
                if (!objectJson.contains("asset"))
                    throw std::runtime_error("Prefab object does not contain an 'asset' field in: " + _rFilePath.string());

                const std::filesystem::path relativeAssetPath = objectJson.at("asset").get<std::string>();
                const std::filesystem::path assetPath = (_rFilePath.parent_path() / relativeAssetPath).lexically_normal();

                const sAssetHandle childAsset = Load(assetPath);

                if (!childAsset.IsValid())
                    throw std::runtime_error("Prefab references invalid asset: " + assetPath.string());

                if (childAsset.type != sAssetType::ShapeModel)
                    throw std::runtime_error("Prefab currently only supports ShapeModel children. Invalid asset: " + assetPath.string());

                sPrefabObjectDesc object{};

                object.modelHandle = static_cast<ShapeModelHandle>(childAsset.handle);

                if (objectJson.contains("position"))
                    object.transform.position = ParseVec3(objectJson.at("position"));
                else
                    object.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);

                if (objectJson.contains("rotation"))
                    object.transform.rotation = ParseVec3(objectJson.at("rotation"));
                else
                    object.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);

                if (objectJson.contains("scale"))
                    object.transform.scale = ParseVec3(objectJson.at("scale"));
                else
                    object.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

                object.generateColliders = objectJson.value("generateColliders", false);
                object.generateLights = objectJson.value("generateLights", true);

                prefabDesc.objects.push_back(std::move(object));
            }

            const PrefabHandle prefabHandle = PrefabManager::CreatePrefab(prefabDesc);

            if (prefabHandle < 0)
                throw std::runtime_error("Failed to create Prefab: " + _rFilePath.string());

            sAssetHandle assetHandle{};

            assetHandle.type = sAssetType::Prefab;
            assetHandle.handle = prefabHandle;

            return assetHandle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cAssetManager::cAssetManager()
            : m_loadedAssets()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cAssetManager::~cAssetManager()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace AssetManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        sAssetHandle Load(const std::filesystem::path& _rFilePath)
        {
            return cAssetManager::GetInstance().Load(_rFilePath);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
