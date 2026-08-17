#include "sceneEditorWindow.h"

#include "graphics/camera.h"

#include "graphics/scene/scene.h"
#include "graphics/scene/sceneLoader.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"

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

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::Update(const Platform::cInput& _rInput, const cCamera& _rCamera)
    {
        if (!m_sceneLoaded || m_pScene == nullptr)
            return;

        if (m_transformMode != eTransformMode::None)
        {
            //if (_rInput.WasKeyPressed(GLFW_KEY_ESCAPE) || _rInput.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
            //{
            //    CancelTransform();
            //    return;
            //}
            //
            //if (_rInput.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            //{
            //    ConfirmTransform();
            //    return;
            //}

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

        //if (ctrlDown && _rInput.WasKeyPressed(GLFW_KEY_S))
        //{
        //    if (m_sceneChanged)
        //        SaveScene(m_scenePath);
        //
        //    return;
        //}
        //
        //if (ImGui::GetIO().WantTextInput)
        //    return;
        //
        //if (ctrlDown && _rInput.WasKeyPressed(GLFW_KEY_D))
        //{
        //    DuplicateSelectedInstance();
        //    return;
        //}
        //
        //if (shiftDown && _rInput.WasKeyPressed(GLFW_KEY_A))
        //{
        //    m_openAddPopup = true;
        //    return;
        //}
        //
        //if (_rInput.WasKeyPressed(GLFW_KEY_DELETE))
        //{
        //    RemoveSelectedInstance();
        //    return;
        //}
        //
        //if (_rInput.WasKeyPressed(GLFW_KEY_G))
        //{
        //    BeginTransform(eTransformMode::Move);
        //    return;
        //}
        //
        //if (_rInput.WasKeyPressed(GLFW_KEY_R))
        //{
        //    BeginTransform(eTransformMode::Rotate);
        //    return;
        //}
        //
        //if (_rInput.WasKeyPressed(GLFW_KEY_F))
        //    BeginTransform(eTransformMode::Scale);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::SetScene(cScene* _pScene, const std::filesystem::path& _rFilePath)
    {
        m_pScene = _pScene;
        m_scenePath = _rFilePath.string();

        if (m_pScene == nullptr)
        {
            m_sceneLoaded = false;
            m_errorMessage = "Scene pointer is null.";
            return;
        }

        sSceneDesc loadedSceneDesc;
        std::string errorMessage;

        if (!SceneLoader::LoadDescFromFile(_rFilePath, loadedSceneDesc, errorMessage))
        {
            m_sceneLoaded = false;
            m_errorMessage = errorMessage;
            return;
        }

        m_sceneDesc = std::move(loadedSceneDesc);

        if (!BuildModelHandleMapFromRuntime())
        {
            m_sceneLoaded = false;
            return;
        }

        m_errorMessage.clear();

        m_sceneLoaded = true;
        m_sceneChanged = false;
        m_previewDirty = false;

        m_selectedInstanceIndex = m_sceneDesc.shapeInstances.empty() ? -1 : 0;

        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;
        m_transformInstanceIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;

        m_openAddPopup = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::SetSceneChangedCallback(SceneChangedCallback _callback)
    {
        m_sceneChangedCallback = std::move(_callback);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::SetOpenModelCallback(OpenModelCallback _callback)
    {
        m_openModelCallback = std::move(_callback);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::OnDraw()
    {
        ImGui::Begin("Scene Editor");

        if (ImGui::BeginTabBar("SceneEditorTabs"))
        {
            if (ImGui::BeginTabItem("Scene"))
            {
                DrawSceneEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Models"))
            {
                DrawModelList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        if (m_previewDirty)
        {
            if (m_sceneChangedCallback)
                m_sceneChangedCallback();

            m_previewDirty = false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::LoadScene(const std::filesystem::path& _rFilePath)
    {
        if (m_pScene == nullptr)
        {
            m_errorMessage = "No runtime scene attached.";
            return;
        }

        cScene loadedScene;
        sSceneDesc loadedSceneDesc;

        std::string errorMessage;

        if (!SceneLoader::LoadDescFromFile(_rFilePath, loadedSceneDesc, errorMessage))
        {
            m_errorMessage = errorMessage;
            return;
        }

        if (!SceneLoader::LoadFromFile(_rFilePath, loadedScene, errorMessage))
        {
            m_errorMessage = errorMessage;
            return;
        }

        *m_pScene = std::move(loadedScene);
        m_sceneDesc = std::move(loadedSceneDesc);
        m_scenePath = _rFilePath.string();

        if (!BuildModelHandleMapFromRuntime())
        {
            m_sceneLoaded = false;
            return;
        }

        m_errorMessage.clear();

        m_sceneLoaded = true;
        m_sceneChanged = false;
        m_previewDirty = true;

        m_selectedInstanceIndex = m_sceneDesc.shapeInstances.empty() ? -1 : 0;

        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;
        m_transformInstanceIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;

        m_openAddPopup = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::SaveScene(const std::filesystem::path& _rFilePath)
    {
        if (!m_sceneLoaded)
            return;

        std::string errorMessage;

        if (!SceneLoader::SaveToFile(_rFilePath, m_sceneDesc, errorMessage))
        {
            m_errorMessage = errorMessage;
            return;
        }

        m_scenePath = _rFilePath.string();
        m_errorMessage.clear();
        m_sceneChanged = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::DrawSceneEditor()
    {
        ImGui::TextUnformatted("Scene File");

        char scenePathBuffer[512];
        std::snprintf(scenePathBuffer, sizeof(scenePathBuffer), "%s", m_scenePath.c_str());

        ImGui::SetNextItemWidth(-160.0f);

        if (ImGui::InputText("##ScenePath", scenePathBuffer, sizeof(scenePathBuffer)))
            m_scenePath = scenePathBuffer;

        ImGui::SameLine();

        if (ImGui::Button("Load"))
            LoadScene(m_scenePath);

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_sceneLoaded || !m_sceneChanged);

        if (ImGui::Button("Save"))
            SaveScene(m_scenePath);

        ImGui::EndDisabled();

        if (m_sceneChanged)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("*");
        }

        if (!m_errorMessage.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("Error: %s", m_errorMessage.c_str());
        }

        if (!m_sceneLoaded)
        {
            ImGui::Separator();
            ImGui::TextDisabled("No scene loaded.");
            return;
        }

        ImGui::Separator();

        ImGui::BeginChild("SceneInstanceList", ImVec2(260.0f, 0.0f), true);
        DrawInstanceList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("SceneInstanceInspector", ImVec2(0.0f, 0.0f), true);
        DrawInspector();
        ImGui::EndChild();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::DrawModelList()
    {
        if (!m_sceneLoaded)
        {
            ImGui::TextDisabled("No scene loaded.");
            return;
        }

        ImGui::TextUnformatted("Scene Models");
        ImGui::Separator();

        for (const sSceneModelDesc& modelDesc : m_sceneDesc.models)
        {
            ImGui::PushID(modelDesc.id.c_str());

            ImGui::TextUnformatted(modelDesc.id.c_str());
            ImGui::SameLine(180.0f);
            ImGui::TextDisabled("%s", modelDesc.filePath.generic_string().c_str());

            ImGui::SameLine();

            if (ImGui::SmallButton("Open"))
            {
                const ShapeModelHandle modelHandle = GetOrLoadModelHandle(modelDesc.id);

                if (modelHandle >= 0 && m_openModelCallback)
                {
                    const std::filesystem::path resolvedPath = (std::filesystem::path(m_scenePath).parent_path() / modelDesc.filePath).lexically_normal();
                    m_openModelCallback(modelHandle, resolvedPath);
                }
            }

            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Model asset add/remove comes later. Instances can already use every model declared in the scene JSON.");
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::DrawInstanceList()
    {
        ImGui::TextUnformatted("Instances");
        ImGui::Separator();

        for (int instanceIndex = 0; instanceIndex < static_cast<int>(m_sceneDesc.shapeInstances.size()); ++instanceIndex)
        {
            const sSceneShapeInstanceDesc& instanceDesc = m_sceneDesc.shapeInstances[instanceIndex];

            const std::string displayName = instanceDesc.name.empty() ? "Instance " + std::to_string(instanceIndex) : instanceDesc.name;

            char label[256];
            std::snprintf(label, sizeof(label), "%s [%s]##SceneInstance%i", displayName.c_str(), instanceDesc.modelId.c_str(), instanceIndex);

            if (ImGui::Selectable(label, instanceIndex == m_selectedInstanceIndex))
                m_selectedInstanceIndex = instanceIndex;
        }

        ImGui::Separator();

        if (ImGui::Button("+ Add"))
            ImGui::OpenPopup("AddSceneInstancePopup");

        if (m_openAddPopup)
        {
            ImGui::OpenPopup("AddSceneInstancePopup");
            m_openAddPopup = false;
        }

        if (ImGui::BeginPopup("AddSceneInstancePopup"))
        {
            for (const sSceneModelDesc& modelDesc : m_sceneDesc.models)
            {
                if (ImGui::MenuItem(modelDesc.id.c_str()))
                    AddInstance(modelDesc.id);
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        const bool hasSelection = HasValidSelection();

        ImGui::BeginDisabled(!hasSelection);

        if (ImGui::Button("Duplicate"))
            DuplicateSelectedInstance();

        ImGui::SameLine();

        if (ImGui::Button("Delete"))
            RemoveSelectedInstance();

        ImGui::EndDisabled();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::DrawInspector()
    {
        if (!HasValidSelection())
        {
            ImGui::TextUnformatted("No scene instance selected.");
            return;
        }

        sSceneShapeInstanceDesc& instanceDesc = m_sceneDesc.shapeInstances[m_selectedInstanceIndex];

        ImGui::Text("%s", instanceDesc.name.empty() ? "Unnamed Instance" : instanceDesc.name.c_str());
        ImGui::Separator();

        // -------------------------------------------------------------------------------------------------------------------------
        // Identity
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Identity", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char nameBuffer[256];
            std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", instanceDesc.name.c_str());

            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            {
                const std::string newName = nameBuffer;

                if (newName.empty() || newName == instanceDesc.name || MakeUniqueInstanceName(newName) == newName)
                {
                    instanceDesc.name = newName;

                    if (RebuildRuntimeScene())
                        MarkSceneChanged();
                }
            }

            int selectedModelIndex = -1;

            for (int modelIndex = 0; modelIndex < static_cast<int>(m_sceneDesc.models.size()); ++modelIndex)
            {
                if (m_sceneDesc.models[modelIndex].id == instanceDesc.modelId)
                {
                    selectedModelIndex = modelIndex;
                    break;
                }
            }

            std::vector<const char*> modelNames;
            modelNames.reserve(m_sceneDesc.models.size());

            for (const sSceneModelDesc& modelDesc : m_sceneDesc.models)
                modelNames.push_back(modelDesc.id.c_str());

            if (!modelNames.empty() && ImGui::Combo("Model", &selectedModelIndex, modelNames.data(), static_cast<int>(modelNames.size())))
            {
                instanceDesc.modelId = m_sceneDesc.models[selectedModelIndex].id;

                if (RebuildRuntimeScene())
                    MarkSceneChanged();
            }

            if (ImGui::Button("Open Model Editor"))
                OpenSelectedModel();
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Transform
        // -------------------------------------------------------------------------------------------------------------------------

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float position[] = { instanceDesc.transform.position.x(), instanceDesc.transform.position.y(), instanceDesc.transform.position.z() };

            if (ImGui::DragFloat3("Position", position, 0.05f))
            {
                instanceDesc.transform.position = Math::cVec3f(position[0], position[1], position[2]);
                ApplySelectedTransformToRuntime();
                MarkSceneChanged();
            }

            float rotation[] = { instanceDesc.transform.rotation.x(), instanceDesc.transform.rotation.y(), instanceDesc.transform.rotation.z() };

            if (ImGui::DragFloat3("Rotation", rotation, 0.01f))
            {
                instanceDesc.transform.rotation = Math::cVec3f(rotation[0], rotation[1], rotation[2]);
                ApplySelectedTransformToRuntime();
                MarkSceneChanged();
            }

            float scale[] = { instanceDesc.transform.scale.x(), instanceDesc.transform.scale.y(), instanceDesc.transform.scale.z() };

            if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.01f, 100.0f))
            {
                instanceDesc.transform.scale = Math::cVec3f(scale[0], scale[1], scale[2]);
                ApplySelectedTransformToRuntime();
                MarkSceneChanged();
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::AddInstance(const std::string& _rModelId)
    {
        if (!_rModelId.empty() && GetOrLoadModelHandle(_rModelId) < 0)
            return;

        sSceneShapeInstanceDesc instanceDesc;

        instanceDesc.name = MakeUniqueInstanceName(_rModelId);
        instanceDesc.modelId = _rModelId;

        instanceDesc.transform.position = Math::cVec3f(0.0f, 0.0f, 0.0f);
        instanceDesc.transform.rotation = Math::cVec3f(0.0f, 0.0f, 0.0f);
        instanceDesc.transform.scale = Math::cVec3f(1.0f, 1.0f, 1.0f);

        m_sceneDesc.shapeInstances.push_back(std::move(instanceDesc));
        m_selectedInstanceIndex = static_cast<int>(m_sceneDesc.shapeInstances.size()) - 1;

        if (RebuildRuntimeScene())
            MarkSceneChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::DuplicateSelectedInstance()
    {
        if (!HasValidSelection())
            return;

        sSceneShapeInstanceDesc instanceDesc = m_sceneDesc.shapeInstances[m_selectedInstanceIndex];

        const std::string baseName = instanceDesc.name.empty() ? instanceDesc.modelId : instanceDesc.name + "_copy";
        instanceDesc.name = MakeUniqueInstanceName(baseName);

        m_sceneDesc.shapeInstances.push_back(std::move(instanceDesc));
        m_selectedInstanceIndex = static_cast<int>(m_sceneDesc.shapeInstances.size()) - 1;

        if (RebuildRuntimeScene())
            MarkSceneChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::RemoveSelectedInstance()
    {
        if (!HasValidSelection())
            return;

        m_sceneDesc.shapeInstances.erase(m_sceneDesc.shapeInstances.begin() + m_selectedInstanceIndex);

        if (m_sceneDesc.shapeInstances.empty())
            m_selectedInstanceIndex = -1;
        else if (m_selectedInstanceIndex >= static_cast<int>(m_sceneDesc.shapeInstances.size()))
            m_selectedInstanceIndex = static_cast<int>(m_sceneDesc.shapeInstances.size()) - 1;

        if (RebuildRuntimeScene())
            MarkSceneChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::OpenSelectedModel()
    {
        if (!HasValidSelection())
            return;

        const sSceneShapeInstanceDesc& instanceDesc = m_sceneDesc.shapeInstances[m_selectedInstanceIndex];
        const sSceneModelDesc* pModelDesc = FindModelDesc(instanceDesc.modelId);

        if (pModelDesc == nullptr)
        {
            m_errorMessage = "Could not find scene model: " + instanceDesc.modelId;
            return;
        }

        const ShapeModelHandle modelHandle = GetOrLoadModelHandle(instanceDesc.modelId);

        if (modelHandle < 0)
            return;

        if (m_openModelCallback)
        {
            const std::filesystem::path resolvedPath = (std::filesystem::path(m_scenePath).parent_path() / pModelDesc->filePath).lexically_normal();
            m_openModelCallback(modelHandle, resolvedPath);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cSceneEditorWindow::HasValidSelection() const
    {
        return m_selectedInstanceIndex >= 0 && m_selectedInstanceIndex < static_cast<int>(m_sceneDesc.shapeInstances.size());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::BeginTransform(eTransformMode _mode)
    {
        if (!HasValidSelection())
            return;

        m_transformMode = _mode;
        m_transformAxis = eTransformAxis::None;

        m_transformInstanceIndex = m_selectedInstanceIndex;
        m_transformStartInstance = m_sceneDesc.shapeInstances[m_selectedInstanceIndex];

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;

        m_transformStartSceneChanged = m_sceneChanged;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::UpdateTransform(const Platform::cInput& _rInput, const cCamera& _rCamera)
    {
        if (m_transformInstanceIndex < 0 || m_transformInstanceIndex >= static_cast<int>(m_sceneDesc.shapeInstances.size()))
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

        sSceneShapeInstanceDesc& instanceDesc = m_sceneDesc.shapeInstances[m_transformInstanceIndex];

        switch (m_transformMode)
        {
        case eTransformMode::Move:
        {
            if (m_transformAxis == eTransformAxis::None)
            {
                Math::cVec3f movement = cameraRight * (mouseDeltaX * c_MoveSpeed);
                movement -= cameraUp * (mouseDeltaY * c_MoveSpeed);

                instanceDesc.transform.position = m_transformStartInstance.transform.position + movement;
            }
            else
            {
                const float movementAmount = getAxisMouseAmount() * c_MoveSpeed;
                instanceDesc.transform.position = m_transformStartInstance.transform.position + axis * movementAmount;
            }

            break;
        }

        case eTransformMode::Rotate:
        {
            if (m_transformAxis == eTransformAxis::None)
            {
                Math::cVec3f rotationDelta = cameraUp * (mouseDeltaX * c_RotationSpeed);
                rotationDelta -= cameraRight * (mouseDeltaY * c_RotationSpeed);

                instanceDesc.transform.rotation = m_transformStartInstance.transform.rotation + rotationDelta;
            }
            else
            {
                const float rotationAmount = getDominantMouseAmount() * c_RotationSpeed;
                instanceDesc.transform.rotation = m_transformStartInstance.transform.rotation + axis * rotationAmount;
            }

            break;
        }

        case eTransformMode::Scale:
        {
            if (m_transformAxis == eTransformAxis::None)
            {
                const float scaleFactor = std::max(0.01f, 1.0f + getDominantMouseAmount() * c_ScaleSpeed);
                instanceDesc.transform.scale = Math::cVec3f(std::max(0.01f, m_transformStartInstance.transform.scale.x() * scaleFactor), std::max(0.01f, m_transformStartInstance.transform.scale.y() * scaleFactor), std::max(0.01f, m_transformStartInstance.transform.scale.z() * scaleFactor));
            }
            else
            {
                const float scaleFactor = std::max(0.01f, 1.0f + getAxisMouseAmount() * c_ScaleSpeed);

                switch (m_transformAxis)
                {
                case eTransformAxis::X:
                    instanceDesc.transform.scale = Math::cVec3f(std::max(0.01f, m_transformStartInstance.transform.scale.x() * scaleFactor), m_transformStartInstance.transform.scale.y(), m_transformStartInstance.transform.scale.z());
                    break;

                case eTransformAxis::Y:
                    instanceDesc.transform.scale = Math::cVec3f(m_transformStartInstance.transform.scale.x(), std::max(0.01f, m_transformStartInstance.transform.scale.y() * scaleFactor), m_transformStartInstance.transform.scale.z());
                    break;

                case eTransformAxis::Z:
                    instanceDesc.transform.scale = Math::cVec3f(m_transformStartInstance.transform.scale.x(), m_transformStartInstance.transform.scale.y(), std::max(0.01f, m_transformStartInstance.transform.scale.z() * scaleFactor));
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

        if (m_pScene != nullptr && m_transformInstanceIndex < static_cast<int>(m_pScene->GetShapeInstances().size()))
            m_pScene->GetShapeInstance(static_cast<SceneShapeInstanceHandle>(m_transformInstanceIndex)).transform = instanceDesc.transform;

        MarkSceneChanged();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::ConfirmTransform()
    {
        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;

        m_transformInstanceIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::CancelTransform()
    {
        if (m_transformInstanceIndex >= 0 && m_transformInstanceIndex < static_cast<int>(m_sceneDesc.shapeInstances.size()))
        {
            m_sceneDesc.shapeInstances[m_transformInstanceIndex] = m_transformStartInstance;
            m_selectedInstanceIndex = m_transformInstanceIndex;

            if (m_pScene != nullptr && m_transformInstanceIndex < static_cast<int>(m_pScene->GetShapeInstances().size()))
                m_pScene->GetShapeInstance(static_cast<SceneShapeInstanceHandle>(m_transformInstanceIndex)).transform = m_transformStartInstance.transform;

            m_sceneChanged = m_transformStartSceneChanged;
            m_previewDirty = true;
        }

        m_transformMode = eTransformMode::None;
        m_transformAxis = eTransformAxis::None;

        m_transformInstanceIndex = -1;

        m_transformMouseDeltaX = 0.0;
        m_transformMouseDeltaY = 0.0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cSceneEditorWindow::BuildModelHandleMapFromRuntime()
    {
        m_modelHandles.clear();

        if (m_pScene == nullptr)
        {
            m_errorMessage = "No runtime scene attached.";
            return false;
        }

        const std::vector<GFX::sShapeInstance>& runtimeInstances = m_pScene->GetShapeInstances();

        if (runtimeInstances.size() != m_sceneDesc.shapeInstances.size())
        {
            m_errorMessage = "Runtime scene and scene description have different instance counts.";
            return false;
        }

        for (size_t instanceIndex = 0; instanceIndex < m_sceneDesc.shapeInstances.size(); ++instanceIndex)
        {
            const std::string& modelId = m_sceneDesc.shapeInstances[instanceIndex].modelId;
            const ShapeModelHandle modelHandle = runtimeInstances[instanceIndex].modelHandle;

            const auto iterator = m_modelHandles.find(modelId);

            if (iterator != m_modelHandles.end() && iterator->second != modelHandle)
            {
                m_errorMessage = "Scene model id '" + modelId + "' maps to multiple runtime handles.";
                return false;
            }

            m_modelHandles[modelId] = modelHandle;
        }

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cSceneEditorWindow::RebuildRuntimeScene()
    {
        if (m_pScene == nullptr)
        {
            m_errorMessage = "No runtime scene attached.";
            return false;
        }

        cScene rebuiltScene;

        for (const sSceneShapeInstanceDesc& instanceDesc : m_sceneDesc.shapeInstances)
        {
            const ShapeModelHandle modelHandle = GetOrLoadModelHandle(instanceDesc.modelId);

            if (modelHandle < 0)
                return false;

            GFX::sShapeInstance shapeInstance{};
            shapeInstance.modelHandle = modelHandle;
            shapeInstance.transform = instanceDesc.transform;

            if (instanceDesc.name.empty())
            {
                rebuiltScene.AddShapeInstance(shapeInstance);
            }
            else if (rebuiltScene.AddNamedShapeInstance(instanceDesc.name, shapeInstance) == c_invalidSceneShapeInstanceHandle)
            {
                m_errorMessage = "Duplicate or invalid scene instance name: " + instanceDesc.name;
                return false;
            }
        }

        *m_pScene = std::move(rebuiltScene);

        m_errorMessage.clear();

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    ShapeModelHandle cSceneEditorWindow::GetOrLoadModelHandle(const std::string& _rModelId)
    {
        const auto handleIterator = m_modelHandles.find(_rModelId);

        if (handleIterator != m_modelHandles.end())
            return handleIterator->second;

        const sSceneModelDesc* pModelDesc = FindModelDesc(_rModelId);

        if (pModelDesc == nullptr)
        {
            m_errorMessage = "Unknown scene model id: " + _rModelId;
            return -1;
        }

        const std::filesystem::path modelPath = (std::filesystem::path(m_scenePath).parent_path() / pModelDesc->filePath).lexically_normal();

        sShapeModelDesc modelDesc;
        std::string errorMessage;

        if (!ShapeModelLoader::LoadFromFile(modelPath, modelDesc, errorMessage))
        {
            m_errorMessage = "Failed to load scene model '" + _rModelId + "': " + errorMessage;
            return -1;
        }

        const ShapeModelHandle modelHandle = ShapeModelManager::CreateShapeModel(modelDesc);

        m_modelHandles.emplace(_rModelId, modelHandle);

        return modelHandle;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sSceneModelDesc* cSceneEditorWindow::FindModelDesc(const std::string& _rModelId) const
    {
        for (const sSceneModelDesc& modelDesc : m_sceneDesc.models)
        {
            if (modelDesc.id == _rModelId)
                return &modelDesc;
        }

        return nullptr;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::ApplySelectedTransformToRuntime()
    {
        if (!HasValidSelection() || m_pScene == nullptr)
            return;

        if (m_selectedInstanceIndex >= static_cast<int>(m_pScene->GetShapeInstances().size()))
            return;

        m_pScene->GetShapeInstance(static_cast<SceneShapeInstanceHandle>(m_selectedInstanceIndex)).transform = m_sceneDesc.shapeInstances[m_selectedInstanceIndex].transform;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    std::string cSceneEditorWindow::MakeUniqueInstanceName(const std::string& _rBaseName) const
    {
        std::string baseName = _rBaseName.empty() ? "instance" : _rBaseName;

        const auto nameExists = [&](const std::string& _rName)
            {
                for (const sSceneShapeInstanceDesc& instanceDesc : m_sceneDesc.shapeInstances)
                {
                    if (instanceDesc.name == _rName)
                        return true;
                }

                return false;
            };

        if (!nameExists(baseName))
            return baseName;

        for (uint32_t suffix = 1; ; ++suffix)
        {
            const std::string candidate = baseName + "_" + std::to_string(suffix);

            if (!nameExists(candidate))
                return candidate;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSceneEditorWindow::MarkSceneChanged()
    {
        m_sceneChanged = true;
        m_previewDirty = true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
