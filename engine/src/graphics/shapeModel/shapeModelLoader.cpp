#include "shapeModelLoader.h"

#include "shapeModelDesc.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        bool ParseMeshType(const std::string& _rMeshTypeName, sMeshTypes::Enum& _rMeshType)
        {
            if (_rMeshTypeName == "Plane")
            {
                _rMeshType = sMeshTypes::Plane;
                return true;
            }

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

            if (_rMeshTypeName == "Sphere")
            {
                _rMeshType = sMeshTypes::Sphere;
                return true;
            }

            if (_rMeshTypeName == "Cylinder")
            {
                _rMeshType = sMeshTypes::Cylinder;
                return true;
            }

            if (_rMeshTypeName == "Cone")
            {
                _rMeshType = sMeshTypes::Cone;
                return true;
            }

            return false;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const char* MeshTypeToString(sMeshTypes::Enum _meshType)
        {
            switch (_meshType)
            {
            case sMeshTypes::Plane:
                return "Plane";

            case sMeshTypes::Cube:
                return "Cube";

            case sMeshTypes::Pyramid:
                return "Pyramid";

            case sMeshTypes::Sphere:
                return "Sphere";

            case sMeshTypes::Cylinder:
                return "Cylinder";

            case sMeshTypes::Cone:
                return "Cone";

            default:
                return "Unknown";
            }
        }

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

        sMaterial ParseMaterial(const nlohmann::json& _rMaterialJson)
        {
            sMaterial material{};

            material.albedo = ParseVec3(_rMaterialJson.at("albedo"));
            material.roughness = _rMaterialJson.at("roughness").get<float>();
            material.metallic = _rMaterialJson.at("metallic").get<float>();

            material.lightWrap = _rMaterialJson.at("lightWrap").get<float>();
            material.shapeContrast = _rMaterialJson.at("shapeContrast").get<float>();
            material.ambientStrength = _rMaterialJson.at("ambientStrength").get<float>();

            material.emissiveColor = ParseVec3(_rMaterialJson.at("emissiveColor"));
            material.emissiveStrength = _rMaterialJson.at("emissiveStrength").get<float>();

            return material;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        nlohmann::json MaterialToJson(const sMaterial& _rMaterial)
        {
            nlohmann::json materialJson;

            materialJson["albedo"] = Vec3ToJson(_rMaterial.albedo);
            materialJson["roughness"] = _rMaterial.roughness;
            materialJson["metallic"] = _rMaterial.metallic;

            materialJson["lightWrap"] = _rMaterial.lightWrap;
            materialJson["shapeContrast"] = _rMaterial.shapeContrast;
            materialJson["ambientStrength"] = _rMaterial.ambientStrength;

            materialJson["emissiveColor"] = Vec3ToJson(_rMaterial.emissiveColor);
            materialJson["emissiveStrength"] = _rMaterial.emissiveStrength;

            return materialJson;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

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

            std::vector<sMaterial> loadedMaterials;

            // -------------------------------------------------------------------------------------------------------------------------
            // Materials
            // -------------------------------------------------------------------------------------------------------------------------

            if (modelJson.contains("materials"))
            {
                const nlohmann::json& materialsJson = modelJson.at("materials");

                for (const nlohmann::json& materialJson : materialsJson)
                    loadedMaterials.push_back(ParseMaterial(materialJson));
            }

            // -------------------------------------------------------------------------------------------------------------------------
            // Shapes
            // -------------------------------------------------------------------------------------------------------------------------

            const nlohmann::json& shapesJson = modelJson.at("shapes");

            for (const nlohmann::json& shapeJson : shapesJson)
            {
                sShapePartDesc shapePart{};

                const std::string meshTypeName = shapeJson.at("meshType").get<std::string>();

                if (!ParseMeshType(meshTypeName, shapePart.meshType))
                {
                    _rErrorMessage = "Unknown mesh type: " + meshTypeName;
                    return false;
                }

                shapePart.transform.position = ParseVec3(shapeJson.at("position"));
                shapePart.transform.rotation = ParseVec3(shapeJson.at("rotation"));
                shapePart.transform.scale = ParseVec3(shapeJson.at("scale"));

                const nlohmann::json& colorJson = shapeJson.at("color");

                shapePart.color[0] = colorJson.at(0).get<float>();
                shapePart.color[1] = colorJson.at(1).get<float>();
                shapePart.color[2] = colorJson.at(2).get<float>();
                shapePart.color[3] = colorJson.at(3).get<float>();

                shapePart.materialIndex = shapeJson.value("materialIndex", 0u);

                loadedModel.shapes.push_back(shapePart);
            }

            _rModelDesc = std::move(loadedModel);

            if (modelJson.contains("materials"))
                MaterialManager::GetMaterials() = std::move(loadedMaterials);

            _rErrorMessage.clear();

            return true;
        }
        catch (const nlohmann::json::exception& _rException)
        {
            _rErrorMessage = "Invalid model JSON: " + std::string(_rException.what());
            return false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool ShapeModelLoader::SaveToFile(const std::filesystem::path& _rFilePath, const sShapeModelDesc& _rModelDesc, std::string& _rErrorMessage)
    {
        std::ofstream file(_rFilePath);

        if (!file.is_open())
        {
            _rErrorMessage = "Could not open model file for writing: " + _rFilePath.string();
            return false;
        }

        try
        {
            nlohmann::json modelJson;

            // -------------------------------------------------------------------------------------------------------------------------
            // Materials
            // -------------------------------------------------------------------------------------------------------------------------

            modelJson["materials"] = nlohmann::json::array();

            const auto& materials = MaterialManager::GetMaterials();

            std::cout << "Material count: " << materials.size() << '\n';

            for (const sMaterial& material : materials)
                modelJson["materials"].push_back(MaterialToJson(material));

            std::cout << "JSON material count: " << modelJson["materials"].size() << '\n';
            std::cout << modelJson.dump(4) << '\n';
            std::cout << "Saving to: " << std::filesystem::absolute(_rFilePath) << '\n';

            // -------------------------------------------------------------------------------------------------------------------------
            // Shapes
            // -------------------------------------------------------------------------------------------------------------------------

            modelJson["shapes"] = nlohmann::json::array();

            for (const sShapePartDesc& shapePart : _rModelDesc.shapes)
            {
                nlohmann::json shapeJson;

                shapeJson["meshType"] = MeshTypeToString(shapePart.meshType);
                shapeJson["position"] = Vec3ToJson(shapePart.transform.position);
                shapeJson["rotation"] = Vec3ToJson(shapePart.transform.rotation);
                shapeJson["scale"] = Vec3ToJson(shapePart.transform.scale);

                shapeJson["color"] =
                {
                    shapePart.color[0],
                    shapePart.color[1],
                    shapePart.color[2],
                    shapePart.color[3]
                };

                shapeJson["materialIndex"] = shapePart.materialIndex;

                modelJson["shapes"].push_back(shapeJson);
            }

            file << modelJson.dump(4) << '\n';

            if (!file.good())
            {
                _rErrorMessage = "Could not write model file: " + _rFilePath.string();
                return false;
            }

            _rErrorMessage.clear();

            return true;
        }
        catch (const nlohmann::json::exception& _rException)
        {
            _rErrorMessage = "Could not create model JSON: " + std::string(_rException.what());
            return false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------