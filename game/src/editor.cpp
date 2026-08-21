#include "editor.h"

#include "graphics/scene/sceneLoader.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/imgui/modelEditorWindow.h"
#include "graphics/imgui/sceneEditorWindow.h"

// -------------------------------------------------------------------------------------------------------------------------

cEditor::cEditor(Engine::sAppConfig& _rAppConfig)
    : cApplication(_rAppConfig)
    , m_planeMesh()
    , m_cubeMesh()
    , m_pyramidMesh()
    , m_sphereMesh()
    , m_cylinderMesh()
    , m_coneMesh()
    , m_playerInstance()
    , m_playerPosition()
    , m_pool()
    , m_instances()
    , m_meshInstances()
    , m_scene()
    , m_playerShapeInstanceHandle()
    , m_modelShapeInstanceHandle()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::OnInit()
{
    using namespace Engine::GFX;

    const std::filesystem::path scenePath = "./assets/scenes/scene.json";

    std::string errorMessage;

    // -------------------------------------------------------------------------------------------------------------------------
    // Scene
    // -------------------------------------------------------------------------------------------------------------------------

    if (!SceneLoader::LoadFromFile(scenePath, m_scene, errorMessage))
    {
        std::cerr << "Failed to load scene: " << errorMessage << '\n';
        return;
    }

    // Scene Editor SOFORT mit der Runtime Scene verbinden
    GetSceneEditorWindow().SetScene(&m_scene, scenePath);

    // Player ist Gameplay-spezifisch und darf den Scene Editor nicht verhindern
    m_playerShapeInstanceHandle = m_scene.FindShapeInstanceHandle("player");

    if (m_playerShapeInstanceHandle == c_invalidSceneShapeInstanceHandle)
        std::cerr << "Warning: Scene does not contain a player instance.\n";

    // -------------------------------------------------------------------------------------------------------------------------
    // Editors
    // -------------------------------------------------------------------------------------------------------------------------

    GetSceneEditorWindow().SetSceneChangedCallback([this]()
        {
            m_playerShapeInstanceHandle = m_scene.FindShapeInstanceHandle("player");

            ClearRenderInstances();
            BuildSceneRenderInstances();
            RebuildInstanceList();
        });

    GetSceneEditorWindow().SetOpenModelCallback([this](ShapeModelHandle _modelHandle, const std::filesystem::path& _rModelPath)
        {
            GetModelEitorWindow().OpenModel(_modelHandle, _rModelPath);
        });

    GetModelEitorWindow().SetModelChangedCallback([this](ShapeModelHandle _modelHandle, const sShapeModelDesc& _rModel)
        {
            ShapeModelManager::UpdateShapeModel(_modelHandle, _rModel);

            ClearRenderInstances();
            BuildSceneRenderInstances();
            RebuildInstanceList();
        });

    // -------------------------------------------------------------------------------------------------------------------------
    // Shape meshes
    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData& planeData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Plane);
    sMeshData& cubeData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    sMeshData& pyramidData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Pyramid);
    sMeshData& sphereData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Sphere);
    sMeshData& cylinderData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cylinder);
    sMeshData& coneData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cone);

    m_planeMesh = CreateMesh(planeData);
    m_cubeMesh = CreateMesh(cubeData);
    m_pyramidMesh = CreateMesh(pyramidData);
    m_sphereMesh = CreateMesh(sphereData);
    m_cylinderMesh = CreateMesh(cylinderData);
    m_coneMesh = CreateMesh(coneData);

    SubmitMesh(m_planeMesh);
    SubmitMesh(m_cubeMesh);
    SubmitMesh(m_pyramidMesh);
    SubmitMesh(m_sphereMesh);
    SubmitMesh(m_cylinderMesh);
    SubmitMesh(m_coneMesh);

    // -------------------------------------------------------------------------------------------------------------------------
    // Build scene render data
    // -------------------------------------------------------------------------------------------------------------------------

    BuildSceneRenderInstances();
    RebuildInstanceList();

    // -------------------------------------------------------------------------------------------------------------------------
    // Main directional light
    // -------------------------------------------------------------------------------------------------------------------------

    sLight directionalLight0{};
    
    directionalLight0.type = sLightType::Directional;
    directionalLight0.color = { 0.8, 0.8, 0.8 };
    directionalLight0.intensity = 2.5f;
    directionalLight0.direction = { -0.5f, -0.5f, -0.3f };
    directionalLight0.castsShadow = true;
    
    LightManager::CreateLight(directionalLight0);
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::OnUpdate(float _deltaTime) 
{
    if (m_hasPendingModelUpdate)
    {
        ApplyEditedModel(m_pendingEditedModel);
        m_hasPendingModelUpdate = false;
    }

    using namespace Engine::Platform;


    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();

    if (IsKeyDown(c_downArrowKey))
    {
        rCamera.AddPitch(-100 * _deltaTime);
    }


    if (IsKeyDown(c_upArrowKey))
    {
        rCamera.AddPitch(100 * _deltaTime);
    }


    if (IsKeyDown(c_leftArrowKey))
    {
        rCamera.AddYaw(-100 * _deltaTime);
    }


    if (IsKeyDown(c_rightArrowKey))
    {
        rCamera.AddYaw(100 * _deltaTime);
    }


    UpdatePlayer(_deltaTime);


    UpdateFreeCam(_deltaTime);


    Engine::GFX::UpdateInstanceBuffer(m_instances);
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::OnDraw()
{
    uint32_t firstInstance = 0;


    for (auto& [mesh, instances] : m_meshInstances)
    {
        Engine::GFX::DrawMeshIntances(mesh, static_cast<uint32_t>(instances.size()), firstInstance);

        firstInstance += static_cast<uint32_t>(instances.size());
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::OnShutdown()
{
    for (auto* pInstance : m_instances)
    {
        m_pool.Destroy(pInstance);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::UpdatePlayer(float _deltaTime)
{
    using namespace Engine::Platform;


    float speed = 5.0f;


    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();


    float direction[4];

    rCamera.GetDirection(direction);


    Engine::Math::cVec3f forward(
        direction[0],
        0.0f,
        direction[2]);


    forward.normalize();


    Engine::Math::cVec3f right(
        -forward.z(),
        0.0f,
        forward.x());


    Engine::Math::cVec3f movement;


    if (IsKeyDown('W'))
    {
        movement += forward * speed * _deltaTime;
    }


    if (IsKeyDown('S'))
    {
        movement -= forward * speed * _deltaTime;
    }


    if (IsKeyDown('A'))
    {
        movement -= right * speed * _deltaTime;
    }


    if (IsKeyDown('D'))
    {
        movement += right * speed * _deltaTime;
    }


    m_playerPosition += movement;

}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::UpdateFreeCam(float _deltaTime)
{
    using namespace Engine::Platform;

    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();

    constexpr float speed = 10.0f;


    float direction[4];
    rCamera.GetDirection(direction);


    Engine::Math::cVec3f forward(
        direction[0],
        direction[1],
        direction[2]
    );

    forward.normalize();


    Engine::Math::cVec3f right(
        -forward.z(),
        0.0f,
        forward.x()
    );

    right.normalize();


    Engine::Math::cVec3f movement;


    if (IsKeyDown('W'))
    {
        movement += forward * speed * _deltaTime;
    }

    if (IsKeyDown('S'))
    {
        movement -= forward * speed * _deltaTime;
    }

    if (IsKeyDown('A'))
    {
        movement -= right * speed * _deltaTime;
    }

    if (IsKeyDown('D'))
    {
        movement += right * speed * _deltaTime;
    }


    if (IsKeyDown('Q'))
    {
        movement += Engine::Math::cVec3f(0.0f, -speed * _deltaTime, 0.0f);
    }

    if (IsKeyDown('E'))
    {
        movement += Engine::Math::cVec3f(0.0f, speed * _deltaTime, 0.0f);
    }


    if (!movement.isZero())
    {
        float position[4];

        rCamera.GetPosition(position);

        Engine::Math::cVec3f cameraPosition(
            position[0],
            position[1],
            position[2]
        );

        cameraPosition += movement;

        rCamera.SetPosition(
            cameraPosition.x(),
            cameraPosition.y(),
            cameraPosition.z()
        );
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::UpdateThirdPersonCamera()
{
    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();


    float distance = 8.0f;


    float direction[4];

    rCamera.GetDirection(direction);


    Engine::Math::cVec3f forward(
        direction[0],
        0.0f,
        direction[2]);


    forward.normalize();


    Engine::Math::cVec3f cameraPosition =
    {
        m_playerPosition.x() - forward.x() * distance,
        m_playerPosition.y() + 5.0f,
        m_playerPosition.z() - forward.z() * distance
    };


    rCamera.SetPosition(
        cameraPosition.x(),
        cameraPosition.y(),
        cameraPosition.z());
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::RebuildInstanceList()
{
    m_instances.clear();


    for (auto& [mesh, instances] : m_meshInstances)
    {
        for (auto* pInstance : instances)
        {
            m_instances.push_back(pInstance);
        }
    }


    std::cout << "GPU Instance order rebuilt: " << m_instances.size() << "\n";
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::QueueEditedModel(const Engine::GFX::sShapeModelDesc& _rModel)
{
    m_pendingEditedModel = _rModel;
    m_hasPendingModelUpdate = true;
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::ApplyEditedModel(const Engine::GFX::sShapeModelDesc& _rModel)
{
    using namespace Engine::GFX;

    const sShapeInstance& playerShapeInstance = m_scene.GetShapeInstance(m_playerShapeInstanceHandle);

    ShapeModelManager::UpdateShapeModel(playerShapeInstance.modelHandle, _rModel);

    ClearRenderInstances();
    BuildSceneRenderInstances();
    RebuildInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::ClearRenderInstances()
{
    for (Engine::GFX::sInstanceData* pInstance : m_instances)
    {
        m_pool.Destroy(pInstance);
    }

    m_instances.clear();
    m_meshInstances.clear();
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::BuildSceneRenderInstances()
{
    for (const Engine::GFX::sShapeInstance& shapeInstance : m_scene.GetShapeInstances())
        BuildRenderInstances(shapeInstance);
}

// -------------------------------------------------------------------------------------------------------------------------

Engine::GFX::MeshHandle cEditor::GetMesh(Engine::GFX::sMeshTypes::Enum _type)
{
    using namespace Engine::GFX;

    switch (_type)
    {
        case sMeshTypes::Plane:
            return m_planeMesh;

        case sMeshTypes::Cube:
            return m_cubeMesh;

        case sMeshTypes::Pyramid:
            return m_pyramidMesh;

        case sMeshTypes::Sphere:
            return m_sphereMesh;

        case sMeshTypes::Cylinder:
            return m_cylinderMesh;

        case sMeshTypes::Cone:
            return m_coneMesh;

        default:
            return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cEditor::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance)
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    const sShapeModelDesc& model = ShapeModelManager::GetShapeModel(_rShapeInstance.modelHandle);

    cMatrix4x4f instanceMatrix = CreateTransformMatrix(_rShapeInstance.transform);

    for (const sShapePartDesc& part : model.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        Math::cMatrix4x4f partMatrix = CreateTransformMatrix(part.transform); 


        pInstance->worldMatrix = partMatrix * instanceMatrix;

        pInstance->color =
        {
            part.color[0],
            part.color[1],
            part.color[2],
            part.color[3]
        };

        pInstance->materialIndex = part.materialIndex;

        MeshHandle mesh = GetMesh(part.meshType);

        m_meshInstances[mesh].push_back(pInstance);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

Math::cMatrix4x4f cEditor::CreateTransformMatrix(const GFX::sTransform& _rTransform)
{
    using namespace Engine::Math;

    cMatrix4x4f translation = cMatrix4x4f::translation(_rTransform.position);

    cMatrix4x4f scale = cMatrix4x4f::scale(_rTransform.scale);

    cMatrix4x4f rotation =
        cMatrix4x4f::rotationX(_rTransform.rotation.x())
        * cMatrix4x4f::rotationY(_rTransform.rotation.y())
        * cMatrix4x4f::rotationZ(_rTransform.rotation.z());

    return scale * rotation * translation;
}

// -------------------------------------------------------------------------------------------------------------------------
