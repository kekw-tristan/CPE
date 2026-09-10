#include "worldModels.h"

#include "graphics/shapeModel/assetManager.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{

    // -------------------------------------------------------------------------------------------------------------------------

    using namespace Engine;

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        class cWorldModels
        {

        public:

            static cWorldModels& GetInstance();


        public:

            bool Load(const std::filesystem::path& _rDirectory);

            GFX::ShapeModelHandle Get(const std::string& _rName) const;

            bool Contains(const std::string& _rName) const;


        private:

            cWorldModels();
            ~cWorldModels();

            cWorldModels(const cWorldModels&) = delete;
            cWorldModels& operator=(const cWorldModels&) = delete;

            cWorldModels(cWorldModels&&) = delete;
            cWorldModels& operator=(cWorldModels&&) = delete;


        private:

            std::unordered_map<std::string, GFX::ShapeModelHandle> m_models;

        };

        // -------------------------------------------------------------------------------------------------------------------------

        cWorldModels& cWorldModels::GetInstance()
        {
            static cWorldModels s_instance;

            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool cWorldModels::Load(const std::filesystem::path& _rDirectory)
        {
            m_models.clear();


            if (!std::filesystem::exists(_rDirectory))
            {
                std::cerr << "World model directory does not exist: " << _rDirectory << '\n';

                return false;
            }


            if (!std::filesystem::is_directory(_rDirectory))
            {
                std::cerr << "World model path is not a directory: " << _rDirectory << '\n';

                return false;
            }


            bool success = true;


            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(_rDirectory))
            {
                if (!entry.is_regular_file())
                    continue;


                // Export metadata and prefab compositions are not standalone shape models.
                if (entry.path().extension() != ".json" || entry.path().filename() == "manifest.json"
                    || entry.path().stem().extension() == ".prefab")
                    continue;


                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), _rDirectory);

                relativePath.replace_extension();


                const std::string modelName = relativePath.generic_string();


                if (m_models.contains(modelName))
                {
                    std::cerr << "Duplicate world model name: " << modelName << '\n';

                    success = false;

                    continue;
                }


                try
                {
                    const GFX::sAssetHandle assetHandle = GFX::AssetManager::Load(entry.path());


                    if (!assetHandle.IsValid())
                    {
                        std::cerr << "Failed to load world model '" << entry.path() << "': Invalid asset handle.\n";

                        success = false;

                        continue;
                    }


                    if (assetHandle.type != GFX::sAssetType::ShapeModel)
                    {
                        std::cerr << "World model asset is not a ShapeModel: " << entry.path() << '\n';

                        success = false;

                        continue;
                    }


                    const GFX::ShapeModelHandle modelHandle = static_cast<GFX::ShapeModelHandle>(assetHandle.handle);


                    m_models.emplace(modelName, modelHandle);


                    std::cout << "Loaded world model: " << modelName << '\n';
                }
                catch (const std::exception& exception)
                {
                    std::cerr << "Failed to load world model '" << entry.path() << "': " << exception.what() << '\n';

                    success = false;
                }
            }


            return success;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        GFX::ShapeModelHandle cWorldModels::Get(const std::string& _rName) const
        {
            const auto iterator = m_models.find(_rName);


            if (iterator == m_models.end())
            {
                std::cerr << "World model not found: " << _rName << '\n';

                return -1;
            }


            return iterator->second;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool cWorldModels::Contains(const std::string& _rName) const
        {
            return m_models.contains(_rName);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cWorldModels::cWorldModels()
            : m_models()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cWorldModels::~cWorldModels()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace WorldModels
    {

        // -------------------------------------------------------------------------------------------------------------------------

        bool Load(const std::filesystem::path& _rDirectory)
        {
            return cWorldModels::GetInstance().Load(_rDirectory);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        GFX::ShapeModelHandle Get(const std::string& _rName)
        {
            return cWorldModels::GetInstance().Get(_rName);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool Contains(const std::string& _rName)
        {
            return cWorldModels::GetInstance().Contains(_rName);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------