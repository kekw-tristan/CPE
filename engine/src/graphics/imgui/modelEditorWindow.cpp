#include "modelEditorWindow.h"

#include "graphics/shapeModel/shapeModelLoader.h"

#include <imgui.h>

#include <cstdio>
#include <utility>

namespace Engine::GFX
{
    void cModelEditorWindow::SetModelChangedCallback(ModelChangedCallback _callback)
    {
        m_modelChangedCallback = std::move(_callback);
    }

    void cModelEditorWindow::OnDraw()
    {
        ImGui::Begin("Model Editor");

        ImGui::Text("File: %s", m_currentFilePath.string().c_str());

        if (ImGui::Button("Load Model"))
        {
            LoadModel(m_currentFilePath);
        }

        if (m_modelChanged)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("Modified");
        }

        if (!m_errorMessage.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("Error: %s", m_errorMessage.c_str());
        }

        if (m_modelLoaded)
        {
            ImGui::Separator();

            ImGui::BeginChild("ShapeList", ImVec2(180.0f, 0.0f), true);
            DrawShapeList();
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("Inspector", ImVec2(0.0f, 0.0f), true);
            DrawInspector();
            ImGui::EndChild();
        }

        ImGui::End();

        if (m_previewDirty && m_modelChangedCallback)
        {
            m_modelChangedCallback(m_model);
            m_previewDirty = false;
        }
    }

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
    }

    void cModelEditorWindow::DrawShapeList()
    {
        ImGui::TextUnformatted("Model Parts");
        ImGui::Separator();

        for (int shapeIndex = 0; shapeIndex < static_cast<int>(m_model.shapes.size()); ++shapeIndex)
        {
            char label[64];
            std::snprintf(label, sizeof(label), "Part %i", shapeIndex);

            if (ImGui::Selectable(label, shapeIndex == m_selectedShapeIndex))
            {
                m_selectedShapeIndex = shapeIndex;
            }
        }
    }

    void cModelEditorWindow::DrawInspector()
    {
        if (m_selectedShapeIndex < 0 || m_selectedShapeIndex >= static_cast<int>(m_model.shapes.size()))
        {
            ImGui::TextUnformatted("No model part selected.");
            return;
        }

        sShapePartDesc& rShape = m_model.shapes[m_selectedShapeIndex];

        ImGui::Text("Part %i", m_selectedShapeIndex);
        ImGui::Separator();

        const char* meshTypeNames[] = { "Cube", "Pyramid" };
        int selectedMeshType = rShape.meshType == sMeshTypes::Cube ? 0 : 1;

        if (ImGui::Combo("Mesh Type", &selectedMeshType, meshTypeNames, 2))
        {
            rShape.meshType = selectedMeshType == 0 ? sMeshTypes::Cube : sMeshTypes::Pyramid;
            MarkModelChanged();
        }

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

        if (ImGui::ColorEdit4("Color", rShape.color))
        {
            MarkModelChanged();
        }
    }

    void cModelEditorWindow::MarkModelChanged()
    {
        m_modelChanged = true;
        m_previewDirty = true;
    }
}