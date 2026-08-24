#pragma once

#include "graphics/shapeModel/shapeModelManager.h"

#include <filesystem>
#include <string>

namespace World
{
	namespace WorldModels
	{
		bool Load(const std::filesystem::path& _rDirectory);

		Engine::GFX::ShapeModelHandle Get(const std::string& _rName);
		bool Contains(const std::string& _rName);
	}
}