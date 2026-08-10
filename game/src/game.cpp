#include "game.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeModelLoader.h"

#include "graphics/imgui/modelEditorWindow.h"

// -------------------------------------------------------------------------------------------------------------------------

cGame::cGame(Engine::sAppConfig& _rAppConfig)
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
    , m_playerShapeInstance()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    using namespace Engine::GFX;

    sShapeModelDesc shapeModelDesc;
    std::string errorMessage;

    if (!ShapeModelLoader::LoadFromFile("./game/assets/models/model.json", shapeModelDesc, errorMessage))
    {
        std::cerr << "Failed to load model: " << errorMessage << '\n';
        return;
    }

    ShapeModelHandle playerHandle = ShapeModelManager::CreateShapeModel(shapeModelDesc);

    sShapeInstance playerInstance =
    {
        .modelHandle = playerHandle,
        .transform =
        {
            .position   = { 0.0f, 0.0f, 0.0f },
            .scale      = { 1.0f, 1.0f, 1.0f },
            .rotation   = { 0.0f, 0.0f, 0.0f }
        }
    };

    sPlaneDesc planeDesc;
    planeDesc.width = 1.0f;
    planeDesc.depth = 1.0f;

    sCubeDesc cubeDesc;
    cubeDesc.width  = 1.0f;
    cubeDesc.height = 1.0f;
    cubeDesc.depth  = 1.0f;


    sPyramidDesc pyramidDesc;
    pyramidDesc.width   = 1.0f;
    pyramidDesc.height  = 1.0f;
    pyramidDesc.depth   = 1.0f;


    sSphereDesc sphereDesc;
    sphereDesc.radius   = 0.5f;
    sphereDesc.segments = 32;
    sphereDesc.rings    = 16;


    sCylinderDesc cylinderDesc;
    cylinderDesc.radius     = 0.5f;
    cylinderDesc.height     = 1.0f;
    cylinderDesc.segments   = 32;


    sConeDesc coneDesc;
    coneDesc.radius     = 0.5f;
    coneDesc.height     = 1.0f;
    coneDesc.segments   = 32;

    sMeshData planeData     = cMeshGenerator::CreatePlane(planeDesc);
    sMeshData cubeData      = cMeshGenerator::CreateCube(cubeDesc);
    sMeshData pyramidData   = cMeshGenerator::CreatePyramid(pyramidDesc);
    sMeshData sphereData    = cMeshGenerator::CreateSphere(sphereDesc);
    sMeshData cylinderData  = cMeshGenerator::CreateCylinder(cylinderDesc);
    sMeshData coneData      = cMeshGenerator::CreateCone(coneDesc);

    m_planeMesh     = CreateMesh(planeData);
    m_cubeMesh      = CreateMesh(cubeData);
    m_pyramidMesh   = CreateMesh(pyramidData);
    m_sphereMesh    = CreateMesh(sphereData);
    m_cylinderMesh  = CreateMesh(cylinderData);
    m_coneMesh      = CreateMesh(coneData);

    SubmitMesh(m_planeMesh);
    SubmitMesh(m_cubeMesh);
    SubmitMesh(m_pyramidMesh);
    SubmitMesh(m_sphereMesh);
    SubmitMesh(m_cylinderMesh);
    SubmitMesh(m_coneMesh);

    m_playerShapeInstance = playerInstance;

    GetModelEitorWindow().SetModelChangedCallback([this](const Engine::GFX::sShapeModelDesc& _rModel) { QueueEditedModel(_rModel); });

    BuildRenderInstances(m_playerShapeInstance);
    RebuildInstanceList();

}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime) 
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

void cGame::OnDraw()
{
    uint32_t firstInstance = 0;


    for (auto& [mesh, instances] : m_meshInstances)
    {
        Engine::GFX::DrawMeshIntances(mesh, static_cast<uint32_t>(instances.size()), firstInstance);

        firstInstance += static_cast<uint32_t>(instances.size());
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnShutdown()
{
    for (auto* pInstance : m_instances)
    {
        m_pool.Destroy(pInstance);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayer(float _deltaTime)
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

void cGame::UpdateFreeCam(float _deltaTime)
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

void cGame::UpdateThirdPersonCamera()
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

void cGame::RebuildInstanceList()
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

void cGame::QueueEditedModel(const Engine::GFX::sShapeModelDesc& _rModel)
{
    m_pendingEditedModel = _rModel;
    m_hasPendingModelUpdate = true;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ApplyEditedModel(const Engine::GFX::sShapeModelDesc& _rModel)
{
    using namespace Engine::GFX;

    sShapeModelDesc& rRuntimeModel = ShapeModelManager::GetShapeModel(m_playerShapeInstance.modelHandle);
    rRuntimeModel = _rModel;

    ClearRenderInstances();
    BuildRenderInstances(m_playerShapeInstance);
    RebuildInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ClearRenderInstances()
{
    for (Engine::GFX::sInstanceData* pInstance : m_instances)
    {
        m_pool.Destroy(pInstance);
    }

    m_instances.clear();
    m_meshInstances.clear();
}

// -------------------------------------------------------------------------------------------------------------------------

Engine::GFX::MeshHandle cGame::GetMesh(Engine::GFX::sMeshTypes::Enum _type)
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

void cGame::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance)
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    sShapeModelDesc& model = ShapeModelManager::GetShapeModel(_rShapeInstance.modelHandle);

    cMatrix4x4f instanceMatrix = CreateTransformMatrix(_rShapeInstance.transform);

    for (const sShapePartDesc& part : model.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        Math::cMatrix4x4f partMatrix = CreateTransformMatrix(part.transform); 


        pInstance->worldMatrix = instanceMatrix * partMatrix;

        pInstance->color =
        {
            part.color[0],
            part.color[1],
            part.color[2],
            part.color[3]
        };

        MeshHandle mesh = GetMesh(part.meshType);

        m_meshInstances[mesh].push_back(pInstance);
    }
}

// -------------------------------------------------------------------------------------------------------------------------

Math::cMatrix4x4f cGame::CreateTransformMatrix(const GFX::sTransform& _rTransform)
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
