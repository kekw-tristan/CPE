#include "game.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "world/worldGenerator.h"

#include <iostream>

// -------------------------------------------------------------------------------------------------------------------------

cGame::cGame(Engine::sAppConfig& _rAppConfig)
    : cApplication(_rAppConfig)
    , m_planeMesh()
    , m_cubeMesh()
    , m_pyramidMesh()
    , m_sphereMesh()
    , m_cylinderMesh()
    , m_coneMesh()
    , m_pool()
    , m_instances()
    , m_meshInstances()
    , m_scene()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    InitMeshes();

    World::WorldGenerator::Generate(m_scene, 1337);

    BuildSceneRenderInstances();
    RebuildInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
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
    ClearRenderInstances();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::InitMeshes()
{
    using namespace Engine::GFX;

    sMeshData& planeData    = ShapeMeshLibrary::GetMeshData(sMeshTypes::Plane);
    sMeshData& cubeData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    sMeshData& pyramidData  = ShapeMeshLibrary::GetMeshData(sMeshTypes::Pyramid);
    sMeshData& sphereData   = ShapeMeshLibrary::GetMeshData(sMeshTypes::Sphere);
    sMeshData& cylinderData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cylinder);
    sMeshData& coneData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cone);

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
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateFreeCam(float _deltaTime)
{
    using namespace Engine::Platform;

    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();

    constexpr float moveSpeed = 10.0f;
    constexpr float rotationSpeed = 100.0f;

    if (IsKeyDown(c_downArrowKey))
        rCamera.AddPitch(-rotationSpeed * _deltaTime);

    if (IsKeyDown(c_upArrowKey))
        rCamera.AddPitch(rotationSpeed * _deltaTime);

    if (IsKeyDown(c_leftArrowKey))
        rCamera.AddYaw(-rotationSpeed * _deltaTime);

    if (IsKeyDown(c_rightArrowKey))
        rCamera.AddYaw(rotationSpeed * _deltaTime);

    float direction[4];
    rCamera.GetDirection(direction);

    Engine::Math::cVec3f forward(direction[0], direction[1], direction[2]);
    forward.normalize();

    Engine::Math::cVec3f right(-forward.z(), 0.0f, forward.x());
    right.normalize();

    Engine::Math::cVec3f movement;

    if (IsKeyDown('W'))
        movement += forward * moveSpeed * _deltaTime;

    if (IsKeyDown('S'))
        movement -= forward * moveSpeed * _deltaTime;

    if (IsKeyDown('A'))
        movement -= right * moveSpeed * _deltaTime;

    if (IsKeyDown('D'))
        movement += right * moveSpeed * _deltaTime;

    if (IsKeyDown('Q'))
        movement += Engine::Math::cVec3f(0.0f, -moveSpeed * _deltaTime, 0.0f);

    if (IsKeyDown('E'))
        movement += Engine::Math::cVec3f(0.0f, moveSpeed * _deltaTime, 0.0f);

    if (movement.isZero())
        return;

    float position[4];
    rCamera.GetPosition(position);

    Engine::Math::cVec3f cameraPosition(position[0], position[1], position[2]);

    cameraPosition += movement;

    rCamera.SetPosition(cameraPosition.x(), cameraPosition.y(), cameraPosition.z());
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BuildSceneRenderInstances()
{
    for (const Engine::GFX::sShapeInstance& shapeInstance : m_scene.GetShapeInstances())
        BuildRenderInstances(shapeInstance);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance)
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    const sShapeModelDesc& model = ShapeModelManager::GetShapeModel(_rShapeInstance.modelHandle);

    cMatrix4x4f instanceMatrix = CreateTransformMatrix(_rShapeInstance.transform);

    for (const sShapePartDesc& part : model.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        cMatrix4x4f partMatrix = CreateTransformMatrix(part.transform);

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

void cGame::RebuildInstanceList()
{
    m_instances.clear();

    for (auto& [mesh, instances] : m_meshInstances)
    {
        for (Engine::GFX::sInstanceData* pInstance : instances)
            m_instances.push_back(pInstance);
    }

    std::cout << "GPU instance order rebuilt: " << m_instances.size() << '\n';
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ClearRenderInstances()
{
    for (Engine::GFX::sInstanceData* pInstance : m_instances)
        m_pool.Destroy(pInstance);

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

Math::cMatrix4x4f cGame::CreateTransformMatrix(const GFX::sTransform& _rTransform)
{
    using namespace Engine::Math;

    cMatrix4x4f translation = cMatrix4x4f::translation(_rTransform.position);
    cMatrix4x4f scale = cMatrix4x4f::scale(_rTransform.scale);

    cMatrix4x4f rotation = cMatrix4x4f::rotationX(_rTransform.rotation.x()) * cMatrix4x4f::rotationY(_rTransform.rotation.y()) * cMatrix4x4f::rotationZ(_rTransform.rotation.z());

    return scale * rotation * translation;
}

// -------------------------------------------------------------------------------------------------------------------------