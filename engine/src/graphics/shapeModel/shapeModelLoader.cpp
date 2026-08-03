#include "shapeModelLoader.h"

#include "shapeModelDesc.h"

#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace Engine::GFX
{
    namespace
    {
        bool ParseMeshType(const std::string& _rMeshTypeName, sMeshTypes::Enum& _rMeshType)
        {
            if (_rMeshTypeName == "Cube")
            {
                _rMeshType = sMeshTypes::Cube;
                return true;
            }

            if (_rMeshTypeName == "Pyramid")
            {
                _rMeshType = sMeshTypes::Pyramid;
                return true;
            }

            return false;
        }

        Math::cVec3f ParseVec3(const nlohmann::json& _rJson)
        {
            return Math::cVec3f(_rJson.at(0).get<float>(), _rJson.at(1).get<float>(), _rJson.at(2).get<float>());
        }
    }

    bool ShapeModelLoader::LoadFromFile(const std::filesystem::path& _rFilePath, sShapeModelDesc& _rModelDesc, std::string& _rErrorMessage)
    {
        std::ifstream file(_rFilePath);

        if (!file.is_open())
        {
            _rErrorMessage = "Could not open model file: " + _rFilePath.string();
            return false;
        }

        try
        {
            nlohmann::json modelJson;
            file >> modelJson;

            sShapeModelDesc loadedModel;
            loadedModel.pDebugName = "";

            const nlohmann::json& shapesJson = modelJson.at("shapes");

            for (const nlohmann::json& shapeJson : shapesJson)
            {
                sShapePartDesc shapePart;

                const std::string meshTypeName = shapeJson.at("meshType").get<std::string>();

                if (!ParseMeshType(meshTypeName, shapePart.meshType))
                {
                    _rErrorMessage = "Unknown mesh type: " + meshTypeName;
                    return false;
                }

                shapePart.transform.position = ParseVec3(shapeJson.at("position"));
                shapePart.transform.rotation = ParseVec3(shapeJson.at("rotation"));
                shapePart.transform.scale    = ParseVec3(shapeJson.at("scale"));

                const nlohmann::json& colorJson = shapeJson.at("color");

            
                shapePart.color[0] = colorJson.at(0).get<float>();
                shapePart.color[1] = colorJson.at(1).get<float>();
                shapePart.color[2] = colorJson.at(2).get<float>();
                shapePart.color[3] = colorJson.at(3).get<float>();
  

                loadedModel.shapes.push_back(shapePart);
            }

            _rModelDesc = std::move(loadedModel);
            _rErrorMessage.clear();

            return true;
        }
        catch (const nlohmann::json::exception& _rException)
        {
            _rErrorMessage = "Invalid model JSON: " + std::string(_rException.what());
            return false;
        }
    }
}