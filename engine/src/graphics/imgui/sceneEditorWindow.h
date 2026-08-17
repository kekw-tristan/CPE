#pragma once

#include "graphics/imgui/imguiWindow.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/scene/sceneDesc.h"

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace Engine::GFX
{
    class cScene;
}

namespace Engine::Platform
{
    class cInput;
}

namespace Engine::GFX
{
    class cCamera;

    class cSceneEditorWindow : public cImGuiWindow
    {
    public:

        using SceneChangedCallback = std::function<void()>;
        using OpenModelCallback = std::function<void(ShapeModelHandle, const std::filesystem::path&)>;

        cSceneEditorWindow() = default;
        ~cSceneEditorWindow() = default;

        cSceneEditorWindow(const cSceneEditorWindow&) = delete;
        cSceneEditorWindow& operator=(const cSceneEditorWindow&) = delete;

        void Update(const Platform::cInput& _rInput, const cCamera& _rCamera);

        void SetScene(cScene* _pScene, const std::filesystem::path& _rFilePath);

        void SetSceneChangedCallback(SceneChangedCallback _callback);
        void SetOpenModelCallback(OpenModelCallback _callback);

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

        void LoadScene(const std::filesystem::path& _rFilePath);
        void SaveScene(const std::filesystem::path& _rFilePath);

        void DrawSceneEditor();
        void DrawModelList();

        void DrawInstanceList();
        void DrawInspector();

        void AddInstance(const std::string& _rModelId);
        void DuplicateSelectedInstance();
        void RemoveSelectedInstance();

        void OpenSelectedModel();

        bool HasValidSelection() const;

        void BeginTransform(eTransformMode _mode);
        void UpdateTransform(const Platform::cInput& _rInput, const cCamera& _rCamera);
        void ConfirmTransform();
        void CancelTransform();

        bool BuildModelHandleMapFromRuntime();
        bool RebuildRuntimeScene();

        ShapeModelHandle GetOrLoadModelHandle(const std::string& _rModelId);
        const sSceneModelDesc* FindModelDesc(const std::string& _rModelId) const;

        void ApplySelectedTransformToRuntime();

        std::string MakeUniqueInstanceName(const std::string& _rBaseName) const;

        void MarkSceneChanged();

    private:

        cScene* m_pScene = nullptr;
        sSceneDesc m_sceneDesc;

        std::unordered_map<std::string, ShapeModelHandle> m_modelHandles;

        std::string m_scenePath = "./assets/scenes/scene.json";
        std::string m_errorMessage;

        SceneChangedCallback m_sceneChangedCallback;
        OpenModelCallback m_openModelCallback;

        int m_selectedInstanceIndex = -1;

        bool m_sceneLoaded = false;
        bool m_sceneChanged = false;
        bool m_previewDirty = false;
        bool m_openAddPopup = false;

        eTransformMode m_transformMode = eTransformMode::None;
        eTransformAxis m_transformAxis = eTransformAxis::None;

        sSceneShapeInstanceDesc m_transformStartInstance;

        int m_transformInstanceIndex = -1;

        double m_transformMouseDeltaX = 0.0;
        double m_transformMouseDeltaY = 0.0;

        bool m_transformStartSceneChanged = false;
    };
}
