#include "modelEditorWindow.h"

#include "graphics/camera.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "platform/input.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

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
            if (m_modelChanged)
                SaveModel(m_modelPath);

            return;
        }

        if (ImGui::GetIO().WantTextInput)
            return;

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

    void cModelEditorWindow::SetModelChangedCallback(ModelChangedCallback _callback)
    {
        m_modelChangedCallback = std::move(_callback);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cModelEditorWindow::OnDraw()
    {
        ImGui::Begin("Model Editor");

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

        ImGui::BeginDisabled(!m_modelLoaded || !m_modelChanged);

        if (ImGui::Button("Save"))
            SaveModel(m_modelPath);

        ImGui::EndDisabled();

        if (m_modelChanged)
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

        ImGui::End();

        if (m_previewDirty && m_modelChangedCallback)
        {
            m_modelChangedCallback(m_model);
            m_previewDirty = false;
        }
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

        m_errorMessage.clear();

        m_modelLoaded = true;
        m_modelChanged = false;
        m_previewDirty = true;

        m_selectedShapeIndex = m_model.shapes.empty() ? -1 : 0;

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

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float position[] =
            {
                rShape.transform.position.x(),
                rShape.transform.position.y(),
                rShape.transform.position.z()
            };

            if (ImGui::DragFloat3("Position", position, 0.05f))
            {
                rShape.transform.position = Math::cVec3f(position[0], position[1], position[2]);
                MarkModelChanged();
            }

            float rotation[] =
            {
                rShape.transform.rotation.x(),
                rShape.transform.rotation.y(),
                rShape.transform.rotation.z()
            };

            if (ImGui::DragFloat3("Rotation", rotation, 0.01f))
            {
                rShape.transform.rotation = Math::cVec3f(rotation[0], rotation[1], rotation[2]);
                MarkModelChanged();
            }

            float scale[] =
            {
                rShape.transform.scale.x(),
                rShape.transform.scale.y(),
                rShape.transform.scale.z()
            };

            if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.01f, 100.0f))
            {
                rShape.transform.scale = Math::cVec3f(scale[0], scale[1], scale[2]);
                MarkModelChanged();
            }
        }

        if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* meshTypeNames[] =
            {
                "Plane",
                "Cube",
                "Pyramid",
                "Sphere",
                "Cylinder",
                "Cone"
            };

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

        m_model.shapes.push_back(shape);
        m_selectedShapeIndex = static_cast<int>(m_model.shapes.size()) - 1;

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

    bool cModelEditorWindow::HasValidSelection() const
    {
        return m_selectedShapeIndex >= 0 && m_selectedShapeIndex < static_cast<int>(m_model.shapes.size());
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

                rShape.transform.scale = Math::cVec3f(std::max(0.01f, m_transformStartShape.transform.scale.x() * scaleFactor),
                    std::max(0.01f, m_transformStartShape.transform.scale.y() * scaleFactor),
                    std::max(0.01f, m_transformStartShape.transform.scale.z() * scaleFactor));
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

}

// -------------------------------------------------------------------------------------------------------------------------