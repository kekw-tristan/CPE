#include "game.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "world/worldGenerator.h"

#include <iostream>

// -------------------------------------------------------------------------------------------------------------------------

cGame::cGame(Engine::sAppConfig& _rAppConfig)
    : cApplication(_rAppConfig)
    , m_planeMesh()
    , m_chunkPlaneMesh()
    , m_cubeMesh()
    , m_pyramidMesh()
    , m_sphereMesh()
    , m_cylinderMesh()
    , m_coneMesh()
    , m_pool()
    , m_instances()
    , m_playerModel()
    , m_playerRenderParts()
    , m_playerPosition()
    , m_playerVelocity()
    , m_isPlayerGrounded()
    , m_playerYaw(0.f)
    , m_cameraPitch(-20.f)
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

    if (LoadPlayerModel())
        BuildPlayerRenderInstances();

    RebuildInstanceList();
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime)
{
    //UpdateFreeCam(_deltaTime);

    UpdatePlayer(_deltaTime);
    UpdatePlayerPhysics(_deltaTime);

    UpdatePlayerRenderInstances();
    UpdateThirdPersonCamera(_deltaTime);

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
    sMeshData& chunkPlane   = ShapeMeshLibrary::GetMeshData(sMeshTypes::ChunkPlane);
    sMeshData& cubeData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    sMeshData& pyramidData  = ShapeMeshLibrary::GetMeshData(sMeshTypes::Pyramid);
    sMeshData& sphereData   = ShapeMeshLibrary::GetMeshData(sMeshTypes::Sphere);
    sMeshData& cylinderData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cylinder);
    sMeshData& coneData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cone);

    m_planeMesh      = CreateMesh(planeData);
    m_chunkPlaneMesh = CreateMesh(chunkPlane);
    m_cubeMesh       = CreateMesh(cubeData);
    m_pyramidMesh    = CreateMesh(pyramidData);
    m_sphereMesh     = CreateMesh(sphereData);
    m_cylinderMesh   = CreateMesh(cylinderData);
    m_coneMesh       = CreateMesh(coneData);

    SubmitMesh(m_planeMesh);
    SubmitMesh(m_chunkPlaneMesh);
    SubmitMesh(m_cubeMesh);
    SubmitMesh(m_pyramidMesh);
    SubmitMesh(m_sphereMesh);
    SubmitMesh(m_cylinderMesh);
    SubmitMesh(m_coneMesh);

    sLight directionalLight0{};
    
    directionalLight0.type          = sLightType::Directional;
    directionalLight0.color         = { 0.8f, 0.8f, 0.8f };
    directionalLight0.intensity     = 2.5f;
    directionalLight0.direction     = { -0.5f, -0.5f, -0.3f };
    directionalLight0.castsShadow   = true;
    
    LightManager::CreateLight(directionalLight0);
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadPlayerModel()
{
    std::string errorMessage;

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/player.json", m_playerModel, errorMessage))
    {
        std::cerr << "Failed to load player model: " << errorMessage << "\n"; 
        return false;
    }

    return true;
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

        if (part.meshType == sMeshTypes::ChunkPlane)
        {
            pInstance->instanceFlags |= sInstanceFlags::InstanceFlagTerrain;
        }

        m_meshInstances[mesh].push_back(pInstance);
    }
}

// -------------------------------------------------------------------------------------------------------------------------


void cGame::BuildPlayerRenderInstances()
{
    using namespace Engine::GFX;

    m_playerRenderParts.clear();
    m_playerRenderParts.reserve(m_playerModel.shapes.size());

    for (const sShapePartDesc& rPart : m_playerModel.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        pInstance->color =
        {
            rPart.color[0],
            rPart.color[1],
            rPart.color[2],
            rPart.color[3]
        };

        pInstance->materialIndex = rPart.materialIndex;

        MeshHandle mesh = GetMesh(rPart.meshType);

        m_meshInstances[mesh].push_back(pInstance);

        sPlayerRenderPart renderPart{};

        renderPart.pInstance = pInstance;
        renderPart.transform = rPart.transform;

        m_playerRenderParts.push_back(renderPart);
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

void cGame::UpdatePlayer(float _deltaTime)
{
    using namespace Engine;
    using namespace Engine::Platform;

    constexpr float c_moveSpeed = 6.0f;
    constexpr float c_jumpVelocity = 6.0f;

    GFX::cCamera& rCamera = GFX::GetCamera();

    float direction[4];
    rCamera.GetDirection(direction);

    Math::cVec3f forward(direction[0], 0.0f, direction[2]);

    if (!forward.isZero())
    {
        forward.normalize();

        Math::cVec3f right(-forward.z(), 0.0f, forward.x());
        right.normalize();

        Math::cVec3f movement;

        if (IsKeyDown('W'))
            movement += forward;

        if (IsKeyDown('S'))
            movement -= forward;

        if (IsKeyDown('A'))
            movement -= right;

        if (IsKeyDown('D'))
            movement += right;

        if (!movement.isZero())
        {
            movement.normalize();

            m_playerPosition += movement * c_moveSpeed * _deltaTime;
            m_playerYaw = std::atan2(movement.x(), movement.z());
        }
    }

    if (IsKeyDown(32) && m_isPlayerGrounded)
    {
        m_playerVelocity = Math::cVec3f(m_playerVelocity.x(), c_jumpVelocity, m_playerVelocity.z());
        m_isPlayerGrounded = false;
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayerPhysics(float _deltaTime)
{
    constexpr float c_gravity = -12.f;
    constexpr float c_groundHeight = 0.0f;

    m_playerVelocity += Math::cVec3f(0.0f, c_gravity * _deltaTime, 0.0f);

    m_playerPosition += Math::cVec3f(0.0f, m_playerVelocity.y() * _deltaTime, 0.0f);

    if (m_playerPosition.y() <= c_groundHeight)
    {
        m_playerPosition = Math::cVec3f(m_playerPosition.x(), c_groundHeight, m_playerPosition.z());
        m_playerVelocity = Math::cVec3f(m_playerVelocity.x(), 0.0f, m_playerVelocity.z());

        m_isPlayerGrounded = true;
    }
}
// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdatePlayerRenderInstances()
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    if (m_playerRenderParts.empty())
        return;

    sTransform playerTransform{};

    playerTransform.position = m_playerPosition;
    playerTransform.rotation = { 0.0f, m_playerYaw, 0.0f };
    playerTransform.scale = { 1.0f, 1.0f, 1.0f };

    const cMatrix4x4f playerMatrix = CreateTransformMatrix(playerTransform);

    for (sPlayerRenderPart& renderPart : m_playerRenderParts)
    {
        const cMatrix4x4f partMatrix = CreateTransformMatrix(renderPart.transform);

        renderPart.pInstance->worldMatrix = partMatrix * playerMatrix;
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateThirdPersonCamera(float _deltaTime)
{
    using namespace Engine;

    constexpr float c_mouseSensitivity  = 0.12f;
    constexpr float c_cameraDistance    = 6.0f;
    constexpr float c_targetHeight      = 1.5f;
    constexpr float c_minPitch          = -60.0f;
    constexpr float c_maxPitch          = -10.0f;

    GFX::cCamera& rCamera = GFX::GetCamera();

    const float mouseDeltaX = static_cast<float>(Platform::GetMouseDeltaX());
    const float mouseDeltaY = static_cast<float>(Platform::GetMouseDeltaY());

    rCamera.AddYaw(mouseDeltaX * c_mouseSensitivity);

    const float pitchChange = -mouseDeltaY * c_mouseSensitivity;
    const float newPitch    = std::clamp(m_cameraPitch + pitchChange, c_minPitch, c_maxPitch);

    rCamera.AddPitch(newPitch - m_cameraPitch);
    m_cameraPitch = newPitch;

    float direction[4];
    rCamera.GetDirection(direction);

    Math::cVec3f cameraDirection(direction[0], direction[1], direction[2]);
    cameraDirection.normalize();

    const Math::cVec3f targetPosition = m_playerPosition + Math::cVec3f(0.0f, c_targetHeight, 0.0f);
    const Math::cVec3f cameraPosition = targetPosition - cameraDirection * c_cameraDistance;

    rCamera.SetPosition(cameraPosition.x(), cameraPosition.y(), cameraPosition.z());
}

// -------------------------------------------------------------------------------------------------------------------------

Engine::GFX::MeshHandle cGame::GetMesh(Engine::GFX::sMeshTypes::Enum _type)
{
    using namespace Engine::GFX;

    switch (_type)
    {
        case sMeshTypes::Plane:
            return m_planeMesh;

        case sMeshTypes::ChunkPlane:
            return m_chunkPlaneMesh;

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
    cMatrix4x4f scale       = cMatrix4x4f::scale(_rTransform.scale);
    cMatrix4x4f rotation    = cMatrix4x4f::rotationX(_rTransform.rotation.x()) * cMatrix4x4f::rotationY(_rTransform.rotation.y()) * cMatrix4x4f::rotationZ(_rTransform.rotation.z());

    return scale * rotation * translation;
}

// -------------------------------------------------------------------------------------------------------------------------