#pragma once

#include "graphics/imgui/imguiWindow.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include <filesystem>
#include <functional>
#include <string>

namespace Engine::Platform
{
    class cInput;
}

namespace Engine::GFX
{
    class cCamera;

    class cModelEditorWindow : public cImGuiWindow
    {
    public:

        using ModelChangedCallback = std::function<void(const sShapeModelDesc&)>;

        cModelEditorWindow() = default;
        ~cModelEditorWindow() = default;

        cModelEditorWindow(const cModelEditorWindow&) = delete;
        cModelEditorWindow& operator=(const cModelEditorWindow&) = delete;

        void Update(const Platform::cInput& _rInput, const cCamera& _rCamera);

        void SetModelChangedCallback(ModelChangedCallback _callback);

    protected:

        void OnDraw() override;

    private:

        enum class eTransformMode
        {
            None,
            Move,
            Rotate,
            Scale
        };

        enum class eTransformAxis
        {
            None,
            X,
            Y,
            Z
        };

    private:

        void LoadModel(const std::filesystem::path& _rFilePath);
        void SaveModel(const std::filesystem::path& _rFilePath);

        void DrawModelEditor();
        void DrawMaterialEditor();

        void DrawShapeList();
        void DrawInspector();

        void DrawMaterialList();
        void DrawMaterialInspector();

        void AddPlane();
        void AddCube();
        void AddPyramid();
        void AddSphere();
        void AddCylinder();
        void AddCone();

        void AddMaterial();

        void DuplicateSelectedShape();
        void RemoveSelectedShape();

        bool HasValidSelection() const;
        bool HasValidMaterialSelection() const;

        void BeginTransform(eTransformMode _mode);
        void UpdateTransform(const Platform::cInput& _rInput, const cCamera& _rCamera);
        void ConfirmTransform();
        void CancelTransform();

        void MarkModelChanged();
        void MarkMaterialChanged();

    private:

        sShapeModelDesc m_model;

        std::string m_modelPath = "./assets/models/model.json";
        std::string m_errorMessage;

        ModelChangedCallback m_modelChangedCallback;

        int m_selectedShapeIndex = -1;
        int m_selectedMaterialIndex = -1;

        bool m_modelLoaded = false;
        bool m_modelChanged = false;
        bool m_materialsChanged = false;
        bool m_previewDirty = false;

        eTransformMode m_transformMode = eTransformMode::None;
        eTransformAxis m_transformAxis = eTransformAxis::None;

        sShapePartDesc m_transformStartShape{};

        int m_transformShapeIndex = -1;

        double m_transformMouseDeltaX = 0.0;
        double m_transformMouseDeltaY = 0.0;

        bool m_transformStartModelChanged = false;
        bool m_openAddPopup = false;
    };
}