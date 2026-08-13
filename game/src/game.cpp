#include "game.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeModelLoader.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

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

    if (!ShapeModelLoader::LoadFromFile("./assets/models/model.json", shapeModelDesc, errorMessage))
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

    // light

   
    // main directional light

    Engine::GFX::sLight directionalLight{};

    directionalLight.type = Engine::GFX::sLightType::Directional;
    directionalLight.color = { 0.2f, 0.2f, 0.2f};
    directionalLight.intensity = 1.5f;
    directionalLight.direction = { -0.4f, -1.0f, -0.3f };


    // blue point light - left front

    Engine::GFX::sLight bluePointLight{};

    bluePointLight.type = Engine::GFX::sLightType::Point;
    bluePointLight.color = { 0.05f, 0.2f, 1.0f };
    bluePointLight.intensity = 10.0f;
    bluePointLight.position = { -8.0f, 2.5f, 6.0f };
    bluePointLight.radius = 7.0f;


    // red point light - right front

    Engine::GFX::sLight redPointLight{};

    redPointLight.type = Engine::GFX::sLightType::Point;
    redPointLight.color = { 1.0f, 0.05f, 0.02f };
    redPointLight.intensity = 9.0f;
    redPointLight.position = { 8.0f, 2.0f, 5.0f };
    redPointLight.radius = 6.0f;


    // green point light - left back

    Engine::GFX::sLight greenPointLight{};

    greenPointLight.type = Engine::GFX::sLightType::Point;
    greenPointLight.color = { 0.05f, 1.0f, 0.15f };
    greenPointLight.intensity = 8.0f;
    greenPointLight.position = { -7.0f, 1.5f, -8.0f };
    greenPointLight.radius = 6.0f;


    // warm point light - right back

    Engine::GFX::sLight warmPointLight{};

    warmPointLight.type = Engine::GFX::sLightType::Point;
    warmPointLight.color = { 1.0f, 0.35f, 0.05f };
    warmPointLight.intensity = 11.0f;
    warmPointLight.position = { 9.0f, 3.0f, -7.0f };
    warmPointLight.radius = 8.0f;


    // purple point light - center far back

    Engine::GFX::sLight purplePointLight{};

    purplePointLight.type = Engine::GFX::sLightType::Point;
    purplePointLight.color = { 0.6f, 0.1f, 1.0f };
    purplePointLight.intensity = 10.0f;
    purplePointLight.position = { 0.0f, 4.0f, -12.0f };
    purplePointLight.radius = 9.0f;


    // cyan point light - center front

    Engine::GFX::sLight cyanPointLight{};

    cyanPointLight.type = Engine::GFX::sLightType::Point;
    cyanPointLight.color = { 0.05f, 0.9f, 1.0f };
    cyanPointLight.intensity = 9.0f;
    cyanPointLight.position = { 0.0f, 2.0f, 11.0f };
    cyanPointLight.radius = 7.0f;


    // spot light - elevated center

    Engine::GFX::sLight spotLight{};

    spotLight.type = Engine::GFX::sLightType::Spot;
    spotLight.color = { 0.7f, 0.8f, 1.0f };
    spotLight.intensity = 15.0f;
    spotLight.position = { 0.0f, 10.0f, 0.0f };
    spotLight.radius = 15.0f;
    spotLight.direction = { 0.0f, -1.0f, 0.0f };
    spotLight.innerCone = 0.966f;
    spotLight.outerCone = 0.906f;


    LightManager::CreateLight(directionalLight);
    LightManager::CreateLight(bluePointLight);
    LightManager::CreateLight(redPointLight);
    LightManager::CreateLight(greenPointLight);
    LightManager::CreateLight(warmPointLight);
    LightManager::CreateLight(purplePointLight);
    LightManager::CreateLight(cyanPointLight);
    LightManager::CreateLight(spotLight);

    // materials

    auto& materials = MaterialManager::GetMaterials();

    materials.clear();

    // -------------------------------------------------------------------------------------------------------------------------
    // Metal
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial metal{};

    metal.albedo = Math::cVec3f(0.55f, 0.57f, 0.60f);
    metal.roughness = 0.25f;
    metal.metallic = 1.0f;
    metal.lightWrap = 0.0f;
    metal.shapeContrast = 1.0f;
    metal.ambientStrength = 0.15f;
    metal.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    metal.emissiveStrength = 0.0f;

    materials.push_back(metal);

    // -------------------------------------------------------------------------------------------------------------------------
    // Plastic
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial plastic{};

    plastic.albedo = Math::cVec3f(0.15f, 0.18f, 0.22f);
    plastic.roughness = 0.35f;
    plastic.metallic = 0.0f;
    plastic.lightWrap = 0.1f;
    plastic.shapeContrast = 1.0f;
    plastic.ambientStrength = 0.2f;
    plastic.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    plastic.emissiveStrength = 0.0f;

    materials.push_back(plastic);

    // -------------------------------------------------------------------------------------------------------------------------
    // Stone
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial stone{};

    stone.albedo = Math::cVec3f(0.38f, 0.36f, 0.32f);
    stone.roughness = 0.85f;
    stone.metallic = 0.0f;
    stone.lightWrap = 0.2f;
    stone.shapeContrast = 1.4f;
    stone.ambientStrength = 0.25f;
    stone.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    stone.emissiveStrength = 0.0f;

    materials.push_back(stone);

    // -------------------------------------------------------------------------------------------------------------------------
    // Wood
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial wood{};

    wood.albedo = Math::cVec3f(0.36f, 0.12f, 0.035f);
    wood.roughness = 0.75f;
    wood.metallic = 0.0f;
    wood.lightWrap = 0.15f;
    wood.shapeContrast = 1.2f;
    wood.ambientStrength = 0.25f;
    wood.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    wood.emissiveStrength = 0.0f;

    materials.push_back(wood);

    // -------------------------------------------------------------------------------------------------------------------------
    // Gold
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial gold{};

    gold.albedo = Math::cVec3f(0.83f, 0.58f, 0.12f);
    gold.roughness = 0.2f;
    gold.metallic = 1.0f;
    gold.lightWrap = 0.0f;
    gold.shapeContrast = 1.0f;
    gold.ambientStrength = 0.15f;
    gold.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    gold.emissiveStrength = 0.0f;

    materials.push_back(gold);

    // -------------------------------------------------------------------------------------------------------------------------
    // Rubber
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial rubber{};

    rubber.albedo = Math::cVec3f(0.025f, 0.025f, 0.025f);
    rubber.roughness = 0.95f;
    rubber.metallic = 0.0f;
    rubber.lightWrap = 0.1f;
    rubber.shapeContrast = 1.5f;
    rubber.ambientStrength = 0.2f;
    rubber.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    rubber.emissiveStrength = 0.0f;

    materials.push_back(rubber);

    // -------------------------------------------------------------------------------------------------------------------------
    // Glass
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial glass{};

    glass.albedo = Math::cVec3f(0.65f, 0.85f, 1.0f);
    glass.roughness = 0.05f;
    glass.metallic = 0.0f;
    glass.lightWrap = 0.0f;
    glass.shapeContrast = 0.5f;
    glass.ambientStrength = 0.1f;
    glass.emissiveColor = Math::cVec3f(0.0f, 0.0f, 0.0f);
    glass.emissiveStrength = 0.0f;

    materials.push_back(glass);

    // -------------------------------------------------------------------------------------------------------------------------
    // Emissive
    // -------------------------------------------------------------------------------------------------------------------------

    sMaterial emissive{};

    emissive.albedo = Math::cVec3f(0.1f, 0.2f, 1.0f);
    emissive.roughness = 0.5f;
    emissive.metallic = 0.0f;
    emissive.lightWrap = 0.0f;
    emissive.shapeContrast = 1.0f;
    emissive.ambientStrength = 0.1f;
    emissive.emissiveColor = Math::cVec3f(0.1f, 0.25f, 1.0f);
    emissive.emissiveStrength = 5.0f;

    materials.push_back(emissive);


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

        pInstance->materialIndex = part.materialIndex;

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
