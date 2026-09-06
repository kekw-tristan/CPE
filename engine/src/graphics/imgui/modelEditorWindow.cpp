#include "modelEditorWindow.h"

#include "graphics/camera.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"

#include "platform/input.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        constexpr float c_MoveSpeed = 0.01f;
        constexpr float c_RotationSpeed = 0.01f;
        constexpr float c_ScaleSpeed = 0.01f;

        // -------------------------------------------------------------------------------------------------------------------------

        const char* GetMeshTypeName(sMeshTypes::Enum _meshType)
        {
            switch (_meshType)
            {
            case sMeshTypes::Plane:
                return "Plane";

            case sMeshTypes::ChunkPlane:
                return "ChunkPlane";

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

            case sMeshTypes::Torus:
                return "Torus";

            case sMeshTypes::Crystal:
                return "Crystal";

            case sMeshTypes::BeveledCube:
                return "BeveledCube";

            case sMeshTypes::Frustum:
                return "Frustum";

            case sMeshTypes::Wedge:
                return "Wedge";

            case sMeshTypes::TriangularPrism:
                return "TriangularPrism";

            case sMeshTypes::IcoSphere:
                return "IcoSphere";

            case sMeshTypes::Rock:
                return "Rock";

            case sMeshTypes::GrassBlade:
                return "GrassBlade";

            case sMeshTypes::Capsule:
                return "Capsule";

            case sMeshTypes::Arch:
                return "Arch";

            case sMeshTypes::ExtrudedPolygon:
                return "ExtrudedPolygon";

            case sMeshTypes::Disc:
                return "Disc";

            case sMeshTypes::Arc:
                return "Arc";

            default:
                return "Unknown";
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::Update(const Platform::cInput& _rInput, const cCamera& _rCamera)
    {
        if (!m_modelLoaded)
            return;

        if (m_transformMode != eTransformMode::None)
        {
            if (_rInput.WasKeyPressed(GLFW_KEY_ESCAPE) || _rInput.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
            {
                CancelTransform();
                return;
            }

            if (_rInput.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                ConfirmTransform();
                return;
            }

            if (_rInput.WasKeyPressed(GLFW_KEY_X))
                m_transformAxis = eTransformAxis::X;
            else if (_rInput.WasKeyPressed(GLFW_KEY_Y))
                m_transformAxis = eTransformAxis::Y;
            else if (_rInput.WasKeyPressed(GLFW_KEY_Z))
                m_transformAxis = eTransformAxis::Z;

            UpdateTransform(_rInput, _rCamera);

            return;
        }

        const bool ctrlDown = _rInput.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || _rInput.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
        const bool shiftDown = _rInput.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || _rInput.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);

        if (ctrlDown && _rInput.WasKeyPressed(GLFW_KEY_S))
        {
            if (m_modelChanged || m_materialsChanged)
                SaveModel(m_modelPath);

            return;
        }

        if (ImGui::GetIO().WantTextInput)
            return;

        if (m_lightTabActive)
        {
            if (ctrlDown && _rInput.WasKeyPressed(GLFW_KEY_D))
                DuplicateSelectedLight();
            else if (_rInput.WasKeyPressed(GLFW_KEY_DELETE))
                RemoveSelectedLight();

            return;
        }

        if (ctrlDown && _rInput.WasKeyPressed(GLFW_KEY_D))
        {
            DuplicateSelectedShape();
            return;
        }

        if (shiftDown && _rInput.WasKeyPressed(GLFW_KEY_A))
        {
            m_openAddPopup = true;
            return;
        }

        if (_rInput.WasKeyPressed(GLFW_KEY_DELETE))
        {
            RemoveSelectedShape();
            return;
        }

        if (_rInput.WasKeyPressed(GLFW_KEY_G))
        {
            BeginTransform(eTransformMode::Move);
            return;
        }

        if (_rInput.WasKeyPressed(GLFW_KEY_R))
        {
            BeginTransform(eTransformMode::Rotate);
            return;
        }

        if (_rInput.WasKeyPressed(GLFW_KEY_F))
            BeginTransform(eTransformMode::Scale);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::OpenModel(ShapeModelHandle _modelHandle, const std::filesystem::path& _rFilePath)
    {
        if (_modelHandle < 0)
        {
            m_errorMessage = "Invalid ShapeModelHandle.";
            return;
        }

        m_modelHandle = _modelHandle;
        m_model = ShapeModelManager::GetShapeModel(_modelHandle);
        m_modelPath = _rFilePath.string();

        m_errorMessage.clear();

        m_modelLoaded = true;
        m_modelChanged = false;
        m_materialsChanged = false;
        m_previewDirty = false;

        m_selectedShapeIndex = m_model.shapes.empty() ? -1 : 0;
        m_selectedMaterialIndex = m_model.materialIndices.empty() ? -1 : 0;
        m_selectedLightIndex = m_model.lights.empty() ? -1 : 0;

        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;
        m_transformShapeIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;

        m_openAddPopup = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::SetModelChangedCallback(ModelChangedCallback _callback)
    {
        m_modelChangedCallback = std::move(_callback);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::OnDraw()
    {
        ImGui::Begin("Model Editor");

        if (ImGui::BeginTabBar("ModelEditorTabs"))
        {
            if (ImGui::BeginTabItem("Model"))
            {
                m_lightTabActive = false;
                DrawModelEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Materials"))
            {
                m_lightTabActive = false;
                DrawMaterialEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Lights"))
            {
                m_lightTabActive = true;
                DrawLightEditor();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        if (m_previewDirty)
        {
            if (m_modelChangedCallback && m_modelHandle >= 0)
                m_modelChangedCallback(m_modelHandle, m_model);

            m_previewDirty = false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawModelEditor()
    {
        ImGui::TextUnformatted("Model File");

        char modelPathBuffer[512];
        std::snprintf(modelPathBuffer, sizeof(modelPathBuffer), "%s", m_modelPath.c_str());

        ImGui::SetNextItemWidth(-160.0f);

        if (ImGui::InputText("##ModelPath", modelPathBuffer, sizeof(modelPathBuffer)))
            m_modelPath = modelPathBuffer;

        ImGui::SameLine();

        if (ImGui::Button("Load"))
            LoadModel(m_modelPath);

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_modelLoaded || (!m_modelChanged && !m_materialsChanged));

        if (ImGui::Button("Save"))
            SaveModel(m_modelPath);

        ImGui::EndDisabled();

        if (m_modelChanged || m_materialsChanged)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("*");
        }

        if (!m_errorMessage.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("Error: %s", m_errorMessage.c_str());
        }

        if (m_modelLoaded)
        {
            ImGui::Separator();

            ImGui::BeginChild("ShapeList", ImVec2(220.0f, 0.0f), true);
            DrawShapeList();
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("Inspector", ImVec2(0.0f, 0.0f), true);
            DrawInspector();
            ImGui::EndChild();
        }
        else
        {
            ImGui::Separator();
            ImGui::TextDisabled("No model loaded.");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawMaterialEditor()
    {
        if (m_model.materialIndices.empty())
            m_selectedMaterialIndex = -1;
        else if (!HasValidMaterialSelection())
            m_selectedMaterialIndex = 0;

        ImGui::BeginChild("MaterialList", ImVec2(220.0f, 0.0f), true);
        DrawMaterialList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("MaterialInspector", ImVec2(0.0f, 0.0f), true);
        DrawMaterialInspector();
        ImGui::EndChild();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawLightEditor()
    {
        if (m_model.lights.empty())
            m_selectedLightIndex = -1;
        else if (!HasValidLightSelection())
            m_selectedLightIndex = 0;

        ImGui::BeginChild("LightList", ImVec2(220.0f, 0.0f), true);
        DrawLightList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("LightInspector", ImVec2(0.0f, 0.0f), true);
        DrawLightInspector();
        ImGui::EndChild();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::LoadModel(const std::filesystem::path& _rFilePath)
    {
        sShapeModelDesc loadedModel;
        std::string errorMessage;

        if (!ShapeModelLoader::LoadFromFile(_rFilePath, loadedModel, errorMessage))
        {
            m_errorMessage = errorMessage;
            return;
        }

        m_model = std::move(loadedModel);
        m_modelHandle = -1;

        m_errorMessage.clear();

        m_modelLoaded = true;
        m_modelChanged = false;
        m_materialsChanged = false;
        m_previewDirty = true;

        m_selectedShapeIndex = m_model.shapes.empty() ? -1 : 0;
        m_selectedMaterialIndex = m_model.materialIndices.empty() ? -1 : 0;
        m_selectedLightIndex = m_model.lights.empty() ? -1 : 0;

        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;
        m_transformShapeIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;

        m_openAddPopup = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::SaveModel(const std::filesystem::path& _rFilePath)
    {
        if (!m_modelLoaded)
            return;

        std::string errorMessage;

        if (!ShapeModelLoader::SaveToFile(_rFilePath, m_model, errorMessage))
        {
            m_errorMessage = errorMessage;
            return;
        }

        m_errorMessage.clear();

        m_modelChanged = false;
        m_materialsChanged = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawShapeList()
    {
        ImGui::TextUnformatted("Model Parts");
        ImGui::Separator();

        for (int shapeIndex = 0; shapeIndex < static_cast<int>(m_model.shapes.size()); ++shapeIndex)
        {
            const sShapePartDesc& rShape = m_model.shapes[shapeIndex];

            char label[128];
            std::snprintf(label, sizeof(label), "%s %i##Shape%i", GetMeshTypeName(rShape.meshType), shapeIndex, shapeIndex);

            if (ImGui::Selectable(label, shapeIndex == m_selectedShapeIndex))
                m_selectedShapeIndex = shapeIndex;
        }

        ImGui::Separator();

        if (ImGui::Button("+ Add"))
            ImGui::OpenPopup("AddShapePopup");

        if (m_openAddPopup)
        {
            ImGui::OpenPopup("AddShapePopup");
            m_openAddPopup = false;
        }

        if (ImGui::BeginPopup("AddShapePopup"))
        {
            if (ImGui::MenuItem("Plane"))
                AddPlane();

            if (ImGui::MenuItem("PlaneChunk"))
                AddPlane();

            if (ImGui::MenuItem("Cube"))
                AddCube();

            if (ImGui::MenuItem("Pyramid"))
                AddPyramid();

            if (ImGui::MenuItem("Sphere"))
                AddSphere();

            if (ImGui::MenuItem("Cylinder"))
                AddCylinder();

            if (ImGui::MenuItem("Cone"))
                AddCone();

            if (ImGui::MenuItem("Torus"))
                AddTorus();

            if (ImGui::MenuItem("Crystal"))
                AddCrystal();

            if (ImGui::MenuItem("BeveledCube"))
            {
                AddShape(sMeshTypes::BeveledCube);
            }

            if (ImGui::MenuItem("Frustum"))
            {
                AddShape(sMeshTypes::Frustum);
            }

            if (ImGui::MenuItem("Wedge"))
            {
                AddShape(sMeshTypes::Wedge);
            }

            if (ImGui::MenuItem("TriangularPrism"))
            {
                AddShape(sMeshTypes::TriangularPrism);
            }

            if (ImGui::MenuItem("IcoSphere"))
            {
                AddShape(sMeshTypes::IcoSphere);
            }

            if (ImGui::MenuItem("Rock"))
            {
                AddShape(sMeshTypes::Rock);
            }

            if (ImGui::MenuItem("GrassBlade"))
            {
                AddShape(sMeshTypes::GrassBlade);
            }

            if (ImGui::MenuItem("Capsule"))
            {
                AddShape(sMeshTypes::Capsule);
            }

            if (ImGui::MenuItem("Arch"))
            {
                AddShape(sMeshTypes::Arch);
            }

            if (ImGui::MenuItem("ExtrudedPolygon"))
            {
                AddShape(sMeshTypes::ExtrudedPolygon);
            }

            if (ImGui::MenuItem("Disc"))
            {
                AddShape(sMeshTypes::Disc);
            }

            if (ImGui::MenuItem("Arc"))
            {
                AddShape(sMeshTypes::Arc);
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        const bool hasSelection = HasValidSelection();

        ImGui::BeginDisabled(!hasSelection);

        if (ImGui::Button("Duplicate"))
            DuplicateSelectedShape();

        ImGui::SameLine();

        if (ImGui::Button("Delete"))
            RemoveSelectedShape();

        ImGui::EndDisabled();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawInspector()
    {
        if (!HasValidSelection())
        {
            ImGui::TextUnformatted("No model part selected.");
            return;
        }

        sShapePartDesc& rShape = m_model.shapes[m_selectedShapeIndex];

        ImGui::Text("%s %i", GetMeshTypeName(rShape.meshType), m_selectedShapeIndex);
        ImGui::Separator();

        // -------------------------------------------------------------------------------------------------------------------------
        // Transform
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float position[] = { rShape.transform.position.x(), rShape.transform.position.y(), rShape.transform.position.z() };

            if (ImGui::DragFloat3("Position", position, 0.05f))
            {
                rShape.transform.position = Math::cVec3f(position[0], position[1], position[2]);
                MarkModelChanged();
            }

            float rotation[] = { rShape.transform.rotation.x(), rShape.transform.rotation.y(), rShape.transform.rotation.z() };

            if (ImGui::DragFloat3("Rotation", rotation, 0.01f))
            {
                rShape.transform.rotation = Math::cVec3f(rotation[0], rotation[1], rotation[2]);
                MarkModelChanged();
            }

            float scale[] = { rShape.transform.scale.x(), rShape.transform.scale.y(), rShape.transform.scale.z() };

            if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.01f, 100.0f))
            {
                rShape.transform.scale = Math::cVec3f(scale[0], scale[1], scale[2]);
                MarkModelChanged();
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Appearance
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* meshTypeNames[] =
            {
                "Plane", "PlaneChunk", "Cube", "Pyramid", "Sphere", "Cylinder", "Cone", "Torus", "Crystal",
                "BeveledCube", "Frustum", "Wedge", "TriangularPrism", "IcoSphere", "Rock",
                "GrassBlade", "Capsule", "Arch", "ExtrudedPolygon", "Disc", "Arc"
            };
            static_assert(sizeof(meshTypeNames) / sizeof(meshTypeNames[0]) == sMeshTypes::NumberOfElements);

            int selectedMeshType = static_cast<int>(rShape.meshType);

            if (selectedMeshType < 0 || selectedMeshType >= static_cast<int>(sMeshTypes::NumberOfElements))
                selectedMeshType = 0;

            if (ImGui::Combo("Mesh Type", &selectedMeshType, meshTypeNames, static_cast<int>(sMeshTypes::NumberOfElements)))
            {
                rShape.meshType = static_cast<sMeshTypes::Enum>(selectedMeshType);
                MarkModelChanged();
            }

            if (ImGui::ColorEdit4("Color", rShape.color))
                MarkModelChanged();
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Material
        // -------------------------------------------------------------------------------------------------------------------------

        ImGui::Separator();
        ImGui::TextUnformatted("Material");

        const std::vector<sMaterial>& materials = MaterialManager::GetMaterials();

        if (m_model.materialIndices.empty())
        {
            ImGui::TextDisabled("No materials in this model.");
            return;
        }

        int localMaterialIndex = 0;

        const auto currentMaterialIterator = std::find(m_model.materialIndices.begin(), m_model.materialIndices.end(), rShape.materialIndex);

        if (currentMaterialIterator == m_model.materialIndices.end())
        {
            rShape.materialIndex = m_model.materialIndices.front();
            MarkModelChanged();
        }
        else
        {
            localMaterialIndex = static_cast<int>(std::distance(m_model.materialIndices.begin(), currentMaterialIterator));
        }

        std::vector<std::string> materialNames;
        materialNames.reserve(m_model.materialIndices.size());

        for (int index = 0; index < static_cast<int>(m_model.materialIndices.size()); ++index)
        {
            const uint32_t globalMaterialIndex = m_model.materialIndices[index];

            if (globalMaterialIndex < materials.size())
                materialNames.push_back("Material " + std::to_string(index));
            else
                materialNames.push_back("Material " + std::to_string(index) + " (invalid)");
        }

        std::vector<const char*> materialNamePointers;
        materialNamePointers.reserve(materialNames.size());

        for (const std::string& name : materialNames)
            materialNamePointers.push_back(name.c_str());

        if (ImGui::Combo("Material", &localMaterialIndex, materialNamePointers.data(), static_cast<int>(materialNamePointers.size())))
        {
            rShape.materialIndex = m_model.materialIndices[localMaterialIndex];
            MarkModelChanged();
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawMaterialList()
    {
        const std::vector<sMaterial>& materials = MaterialManager::GetMaterials();

        ImGui::TextUnformatted("Materials");
        ImGui::Separator();

        for (int localMaterialIndex = 0; localMaterialIndex < static_cast<int>(m_model.materialIndices.size()); ++localMaterialIndex)
        {
            const uint32_t globalMaterialIndex = m_model.materialIndices[localMaterialIndex];

            char label[128];

            if (globalMaterialIndex < materials.size())
                std::snprintf(label, sizeof(label), "Material %i##Material%i", localMaterialIndex, localMaterialIndex);
            else
                std::snprintf(label, sizeof(label), "Material %i (invalid)##Material%i", localMaterialIndex, localMaterialIndex);

            if (ImGui::Selectable(label, localMaterialIndex == m_selectedMaterialIndex))
                m_selectedMaterialIndex = localMaterialIndex;
        }

        ImGui::Separator();

        if (ImGui::Button("+ Add"))
            AddMaterial();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawMaterialInspector()
    {
        if (!HasValidMaterialSelection())
        {
            ImGui::TextUnformatted("No material selected.");
            return;
        }

        std::vector<sMaterial>& materials = MaterialManager::GetMaterials();

        const uint32_t globalMaterialIndex = m_model.materialIndices[m_selectedMaterialIndex];

        if (globalMaterialIndex >= materials.size())
        {
            ImGui::TextUnformatted("Selected material is invalid.");
            return;
        }

        sMaterial& rMaterial = materials[globalMaterialIndex];

        ImGui::Text("Material %i", m_selectedMaterialIndex);
        ImGui::Separator();


        // -------------------------------------------------------------------------------------------------------------------------
        // Surface
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float albedo[] = { rMaterial.albedo.x(), rMaterial.albedo.y(), rMaterial.albedo.z() };

            if (ImGui::ColorEdit3("Albedo", albedo))
            {
                rMaterial.albedo = Math::cVec3f(albedo[0], albedo[1], albedo[2]);
                MarkMaterialChanged();
            }

            if (ImGui::DragFloat("Roughness", &rMaterial.roughness, 0.01f, 0.0f, 1.0f))
                MarkMaterialChanged();

            if (ImGui::DragFloat("Metallic", &rMaterial.metallic, 0.01f, 0.0f, 1.0f))
                MarkMaterialChanged();
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Lighting
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat("Light Wrap", &rMaterial.lightWrap, 0.01f, 0.0f, 1.0f))
                MarkMaterialChanged();

            if (ImGui::DragFloat("Shape Contrast", &rMaterial.shapeContrast, 0.01f, 0.0f, 10.0f))
                MarkMaterialChanged();

            if (ImGui::DragFloat("Ambient Strength", &rMaterial.ambientStrength, 0.01f, 0.0f, 10.0f))
                MarkMaterialChanged();
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Emissive
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Emissive", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float emissiveColor[] = { rMaterial.emissiveColor.x(), rMaterial.emissiveColor.y(), rMaterial.emissiveColor.z() };

            if (ImGui::ColorEdit3("Emissive Color", emissiveColor))
            {
                rMaterial.emissiveColor = Math::cVec3f(emissiveColor[0], emissiveColor[1], emissiveColor[2]);
                MarkMaterialChanged();
            }

            if (ImGui::DragFloat("Emissive Strength", &rMaterial.emissiveStrength, 0.01f, 0.0f, 100.0f))
                MarkMaterialChanged();
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawLightList()
    {
        ImGui::TextUnformatted("Model Lights");
        ImGui::Separator();

        for (int lightIndex = 0; lightIndex < static_cast<int>(m_model.lights.size()); ++lightIndex)
        {
            const sShapeLightDesc& light = m_model.lights[lightIndex];
            const char* pTypeName = light.type == sLightType::Spot ? "Spot" : "Point";

            char label[192];
            std::snprintf(label, sizeof(label), "%s (%s)##Light%i", light.name.c_str(), pTypeName, lightIndex);

            if (ImGui::Selectable(label, lightIndex == m_selectedLightIndex))
                m_selectedLightIndex = lightIndex;
        }

        ImGui::Separator();

        if (ImGui::Button("+ Point"))
            AddLight(sLightType::Point);

        ImGui::SameLine();

        if (ImGui::Button("+ Spot"))
            AddLight(sLightType::Spot);

        const bool hasSelection = HasValidLightSelection();

        ImGui::BeginDisabled(!hasSelection);

        if (ImGui::Button("Duplicate"))
            DuplicateSelectedLight();

        ImGui::SameLine();

        if (ImGui::Button("Delete"))
            RemoveSelectedLight();

        ImGui::EndDisabled();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DrawLightInspector()
    {
        if (!HasValidLightSelection())
        {
            ImGui::TextUnformatted("No model light selected.");
            return;
        }

        constexpr float c_degreesToRadians = 0.01745329252f;
        constexpr float c_radiansToDegrees = 57.2957795131f;

        sShapeLightDesc& light = m_model.lights[m_selectedLightIndex];

        char nameBuffer[128];
        std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", light.name.c_str());

        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
        {
            light.name = nameBuffer;
            MarkModelChanged();
        }

        const char* lightTypeNames[] = { "Point", "Spot" };
        int selectedLightType = light.type == sLightType::Spot ? 1 : 0;

        if (ImGui::Combo("Type", &selectedLightType, lightTypeNames, 2))
        {
            light.type = selectedLightType == 1 ? sLightType::Spot : sLightType::Point;
            MarkModelChanged();
        }

        float position[] = { light.position.x(), light.position.y(), light.position.z() };

        if (ImGui::DragFloat3("Position", position, 0.05f))
        {
            light.position = Math::cVec3f(position[0], position[1], position[2]);
            MarkModelChanged();
        }

        float color[] = { light.color.x(), light.color.y(), light.color.z() };

        if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
        {
            light.color = Math::cVec3f(color[0], color[1], color[2]);
            MarkModelChanged();
        }

        if (ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 1000.0f))
            MarkModelChanged();

        if (ImGui::DragFloat("Radius", &light.radius, 0.05f, 0.01f, 1000.0f))
            MarkModelChanged();

        if (light.type == sLightType::Spot)
        {
            float direction[] = { light.direction.x(), light.direction.y(), light.direction.z() };

            if (ImGui::DragFloat3("Direction", direction, 0.01f))
            {
                light.direction = Math::cVec3f(direction[0], direction[1], direction[2]);
                MarkModelChanged();
            }

            float innerConeDegrees = light.innerConeAngle * c_radiansToDegrees;
            float outerConeDegrees = light.outerConeAngle * c_radiansToDegrees;

            if (ImGui::DragFloat("Inner Cone", &innerConeDegrees, 0.25f, 0.0f, 89.0f, "%.1f deg"))
            {
                innerConeDegrees = std::clamp(innerConeDegrees, 0.0f, std::max(0.0f, outerConeDegrees - 0.1f));
                light.innerConeAngle = innerConeDegrees * c_degreesToRadians;
                MarkModelChanged();
            }

            if (ImGui::DragFloat("Outer Cone", &outerConeDegrees, 0.25f, 0.1f, 89.0f, "%.1f deg"))
            {
                outerConeDegrees = std::clamp(outerConeDegrees, innerConeDegrees + 0.1f, 89.0f);
                light.outerConeAngle = outerConeDegrees * c_degreesToRadians;
                MarkModelChanged();
            }
        }

        if (ImGui::Checkbox("Casts Shadow", &light.castsShadow))
            MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddPlane()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Plane;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddCube()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Cube;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddPyramid()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Pyramid;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddSphere()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Sphere;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddCylinder()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Cylinder;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddCone()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Cone;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddTorus()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Torus;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddCrystal()
    {
        sShapePartDesc shape{};

        shape.meshType = sMeshTypes::Crystal;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddShape(sMeshTypes::Enum _meshType)
    {
        sShapePartDesc shape{};

        shape.meshType = _meshType;

        shape.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        shape.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        shape.color[0] = 1.0f;
        shape.color[1] = 1.0f;
        shape.color[2] = 1.0f;
        shape.color[3] = 1.0f;

        shape.materialIndex = EnsureDefaultMaterial();

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddMaterial()
    {
        sMaterial material{};

        const uint32_t globalMaterialIndex = static_cast<uint32_t>(MaterialManager::CreateMaterial(material));

        m_model.materialIndices.push_back(globalMaterialIndex);
        m_selectedMaterialIndex = static_cast<int>(m_model.materialIndices.size()) - 1;

        MarkMaterialChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::AddLight(sLightType::Enum _lightType)
    {
        sShapeLightDesc light{};

        light.type = _lightType;

        int lightNumber = static_cast<int>(m_model.lights.size()) + 1;

        do
        {
            light.name = "Light " + std::to_string(lightNumber++);
        }
        while (std::any_of(m_model.lights.begin(), m_model.lights.end(), [&light](const sShapeLightDesc& _rExistingLight)
        {
            return _rExistingLight.name == light.name;
        }));

        m_model.lights.push_back(light);
        m_selectedLightIndex = static_cast<int>(m_model.lights.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DuplicateSelectedShape()
    {
        if (!HasValidSelection())
            return;

        const sShapePartDesc shape = m_model.shapes[m_selectedShapeIndex];

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::RemoveSelectedShape()
    {
        if (!HasValidSelection())
            return;

        m_model.shapes.erase(m_model.shapes.begin() + m_selectedShapeIndex);

        if (m_model.shapes.empty())
            m_selectedShapeIndex = -1;
        else if (m_selectedShapeIndex >= static_cast<int>(m_model.shapes.size()))
            m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::DuplicateSelectedLight()
    {
        if (!HasValidLightSelection())
            return;

        sShapeLightDesc light = m_model.lights[m_selectedLightIndex];
        const std::string baseName = light.name.empty() ? "Light" : light.name;
        int copyNumber = 2;

        do
        {
            light.name = baseName + " " + std::to_string(copyNumber++);
        }
        while (std::any_of(m_model.lights.begin(), m_model.lights.end(), [&light](const sShapeLightDesc& _rExistingLight)
        {
            return _rExistingLight.name == light.name;
        }));

        m_model.lights.push_back(light);
        m_selectedLightIndex = static_cast<int>(m_model.lights.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::RemoveSelectedLight()
    {
        if (!HasValidLightSelection())
            return;

        m_model.lights.erase(m_model.lights.begin() + m_selectedLightIndex);

        if (m_model.lights.empty())
            m_selectedLightIndex = -1;
        else if (m_selectedLightIndex >= static_cast<int>(m_model.lights.size()))
            m_selectedLightIndex = static_cast<int>(m_model.lights.size()) - 1;

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cModelEditorWindow::HasValidSelection() const
    {
        return m_selectedShapeIndex >= 0 && m_selectedShapeIndex < static_cast<int>(m_model.shapes.size());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cModelEditorWindow::HasValidMaterialSelection() const
    {
        if (m_selectedMaterialIndex < 0 || m_selectedMaterialIndex >= static_cast<int>(m_model.materialIndices.size()))
            return false;

        const std::vector<sMaterial>& materials = MaterialManager::GetMaterials();
        const uint32_t globalMaterialIndex = m_model.materialIndices[m_selectedMaterialIndex];

        return globalMaterialIndex < materials.size();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cModelEditorWindow::HasValidLightSelection() const
    {
        return m_selectedLightIndex >= 0 && m_selectedLightIndex < static_cast<int>(m_model.lights.size());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::BeginTransform(eTransformMode _mode)
    {
        if (!HasValidSelection())
            return;

        m_transformMode = _mode;
        m_transformAxis = eTransformAxis::None;

        m_transformShapeIndex = m_selectedShapeIndex;
        m_transformStartShape = m_model.shapes[m_selectedShapeIndex];

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;

        m_transformStartModelChanged = m_modelChanged;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::UpdateTransform(const Platform::cInput& _rInput, const cCamera& _rCamera)
    {
        if (m_transformShapeIndex < 0 || m_transformShapeIndex >= static_cast<int>(m_model.shapes.size()))
        {
            ConfirmTransform();
            return;
        }

        m_transformMouseDeltaX += _rInput.GetMouseDeltaX();
        m_transformMouseDeltaY += _rInput.GetMouseDeltaY();

        const float mouseDeltaX = static_cast<float>(m_transformMouseDeltaX);
        const float mouseDeltaY = static_cast<float>(m_transformMouseDeltaY);

        if (std::abs(mouseDeltaX) < 0.0001f && std::abs(mouseDeltaY) < 0.0001f)
            return;

        float direction[4];
        _rCamera.GetDirection(direction);

        Math::cVec3f cameraForward(direction[0], direction[1], direction[2]);

        if (cameraForward.isZero())
            return;

        cameraForward.normalize();

        const Math::cVec3f worldUp(0.0f, 1.0f, 0.0f);

        Math::cVec3f cameraRight = cameraForward.cross(worldUp);

        if (cameraRight.isZero())
            cameraRight = Math::cVec3f(1.0f, 0.0f, 0.0f);
        else
            cameraRight.normalize();

        Math::cVec3f cameraUp = cameraRight.cross(cameraForward);
        cameraUp.normalize();

        Math::cVec3f axis(0.0f, 0.0f, 0.0f);

        switch (m_transformAxis)
        {
        case eTransformAxis::X:
            axis = Math::cVec3f(1.0f, 0.0f, 0.0f);
            break;

        case eTransformAxis::Y:
            axis = Math::cVec3f(0.0f, 1.0f, 0.0f);
            break;

        case eTransformAxis::Z:
            axis = Math::cVec3f(0.0f, 0.0f, 1.0f);
            break;

        case eTransformAxis::None:
            break;
        }

        const auto getDominantMouseAmount = [&]()
            {
                return std::abs(mouseDeltaX) >= std::abs(mouseDeltaY) ? mouseDeltaX : -mouseDeltaY;
            };

        const auto getAxisMouseAmount = [&]()
            {
                const float screenAxisX = axis.dot(cameraRight);
                const float screenAxisY = -axis.dot(cameraUp);

                const float screenAxisLength = std::sqrt(screenAxisX * screenAxisX + screenAxisY * screenAxisY);

                if (screenAxisLength < 0.001f)
                    return getDominantMouseAmount();

                return (mouseDeltaX * screenAxisX + mouseDeltaY * screenAxisY) / screenAxisLength;
            };

        sShapePartDesc& rShape = m_model.shapes[m_transformShapeIndex];

        switch (m_transformMode)
        {
        case eTransformMode::Move:
        {
            if (m_transformAxis == eTransformAxis::None)
            {
                Math::cVec3f movement = cameraRight * (mouseDeltaX * c_MoveSpeed);
                movement -= cameraUp * (mouseDeltaY * c_MoveSpeed);

                rShape.transform.position = m_transformStartShape.transform.position + movement;
            }
            else
            {
                const float movementAmount = getAxisMouseAmount() * c_MoveSpeed;
                rShape.transform.position = m_transformStartShape.transform.position + axis * movementAmount;
            }

            break;
        }

        case eTransformMode::Rotate:
        {
            if (m_transformAxis == eTransformAxis::None)
            {
                Math::cVec3f rotationDelta = cameraUp * (mouseDeltaX * c_RotationSpeed);
                rotationDelta -= cameraRight * (mouseDeltaY * c_RotationSpeed);

                rShape.transform.rotation = m_transformStartShape.transform.rotation + rotationDelta;
            }
            else
            {
                const float rotationAmount = getDominantMouseAmount() * c_RotationSpeed;
                rShape.transform.rotation = m_transformStartShape.transform.rotation + axis * rotationAmount;
            }

            break;
        }

        case eTransformMode::Scale:
        {
            if (m_transformAxis == eTransformAxis::None)
            {
                const float scaleFactor = std::max(0.01f, 1.0f + getDominantMouseAmount() * c_ScaleSpeed);
                rShape.transform.scale = Math::cVec3f(std::max(0.01f, m_transformStartShape.transform.scale.x() * scaleFactor), std::max(0.01f, m_transformStartShape.transform.scale.y() * scaleFactor), std::max(0.01f, m_transformStartShape.transform.scale.z() * scaleFactor));
            }
            else
            {
                const float scaleFactor = std::max(0.01f, 1.0f + getAxisMouseAmount() * c_ScaleSpeed);

                switch (m_transformAxis)
                {
                case eTransformAxis::X:
                    rShape.transform.scale = Math::cVec3f(std::max(0.01f, m_transformStartShape.transform.scale.x() * scaleFactor), m_transformStartShape.transform.scale.y(), m_transformStartShape.transform.scale.z());
                    break;

                case eTransformAxis::Y:
                    rShape.transform.scale = Math::cVec3f(m_transformStartShape.transform.scale.x(), std::max(0.01f, m_transformStartShape.transform.scale.y() * scaleFactor), m_transformStartShape.transform.scale.z());
                    break;

                case eTransformAxis::Z:
                    rShape.transform.scale = Math::cVec3f(m_transformStartShape.transform.scale.x(), m_transformStartShape.transform.scale.y(), std::max(0.01f, m_transformStartShape.transform.scale.z() * scaleFactor));
                    break;

                case eTransformAxis::None:
                    break;
                }
            }

            break;
        }

        case eTransformMode::None:
            return;
        }

        MarkModelChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::ConfirmTransform()
    {
        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;

        m_transformShapeIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::CancelTransform()
    {
        if (m_transformShapeIndex >= 0 && m_transformShapeIndex < static_cast<int>(m_model.shapes.size()))
        {
            m_model.shapes[m_transformShapeIndex] = m_transformStartShape;
            m_selectedShapeIndex = m_transformShapeIndex;

            m_modelChanged = m_transformStartModelChanged;
            m_previewDirty = true;
        }

        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;

        m_transformShapeIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::MarkModelChanged()
    {
        m_modelChanged = true;
        m_previewDirty = true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::MarkMaterialChanged()
    {
        m_materialsChanged = true;

        if (m_modelLoaded)
            m_previewDirty = true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cModelEditorWindow::EnsureDefaultMaterial()
    {
        if (!m_model.materialIndices.empty())
            return m_model.materialIndices.front();

        sMaterial material{};

        const uint32_t globalMaterialIndex = static_cast<uint32_t>(MaterialManager::CreateMaterial(material));

        m_model.materialIndices.push_back(globalMaterialIndex);
        m_selectedMaterialIndex = 0;
        m_materialsChanged = true;

        return globalMaterialIndex;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
