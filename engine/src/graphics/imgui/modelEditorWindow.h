#pragma once

#include "graphics/imgui/imguiWindow.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include <filesystem>
#include <functional>
#include <string>

namespace Engine::GFX
{
    class cModelEditorWindow : public cImGuiWindow
    {
    public:
        using ModelChangedCallback = std::function<void(const sShapeModelDesc&)>;

        cModelEditorWindow() = default;
        ~cModelEditorWindow() = default;

        cModelEditorWindow(const cModelEditorWindow&) = delete;
        cModelEditorWindow& operator=(const cModelEditorWindow&) = delete;

        void SetModelChangedCallback(ModelChangedCallback _callback);

    protected:
        void OnDraw() override;

    private:
        void LoadModel(const std::filesystem::path& _rFilePath);
        void DrawShapeList();
        void DrawInspector();
        void MarkModelChanged();

    private:
        sShapeModelDesc m_model;

        std::filesystem::path m_currentFilePath = "./game/assets/models/model.json";
        std::string m_errorMessage;

        ModelChangedCallback m_modelChangedCallback;

        int m_selectedShapeIndex = -1;

        bool m_modelLoaded = false;
        bool m_modelChanged = false;
        bool m_previewDirty = false;
    };
}