#include "worldModels.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"

#include <iostream>
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

				cWorldModels(const cWorldModels&)				= delete; 
				cWorldModels& operator=(const cWorldModels&)	= delete;

				cWorldModels(const cWorldModels&&)				= delete;
				cWorldModels& operator=(const cWorldModels&&)	= delete;

			private:

				std::unordered_map<std::string, GFX::ShapeModelHandle> m_models;
		};

		// -------------------------------------------------------------------------------------------------------------------------
	}

	// -------------------------------------------------------------------------------------------------------------------------

	namespace
	{

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

				if (entry.path().extension() != ".json")
					continue;

				GFX::sShapeModelDesc model{};
				std::string errorMessage;

				if (!GFX::ShapeModelLoader::LoadFromFile(entry.path(), model, errorMessage))
				{
					std::cerr << "Failed to load world model '" << entry.path() << "': " << errorMessage << '\n';
					success = false;
					continue;
				}

				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), _rDirectory);
				relativePath.replace_extension();

				const std::string modelName = relativePath.generic_string();

				if (m_models.contains(modelName))
				{
					std::cerr << "Duplicate world model name: " << modelName << '\n';
					success = false;
					continue;
				}

				GFX::ShapeModelHandle modelHandle = GFX::ShapeModelManager::CreateShapeModel(model);

				m_models.emplace(modelName, modelHandle);

				std::cout << "Loaded world model: " << modelName << '\n';
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
			return m_models.contains(_rName);;
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
