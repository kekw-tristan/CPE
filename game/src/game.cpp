#include "game.h"

// -------------------------------------------------------------------------------------------------------------------------

cGame::cGame(Engine::sAppConfig& _rAppConfig)
    : cApplication(_rAppConfig)
    , m_cubeMesh()
    , m_pyramidMesh()
    , m_playerInstance()
    , m_playerPosition()
    , m_pool()
    , m_instances()
    , m_meshInstances()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnInit()
{
    Engine::GFX::sCubeDesc cubeDesc;

    cubeDesc.width = 1.0f;
    cubeDesc.depth = 1.0f;
    cubeDesc.height = 1.0f;


    Engine::GFX::sPyramidDesc pyramidDesc;

    pyramidDesc.baseCornerCount = 4;
    pyramidDesc.baseRadius = 0.5f;
    pyramidDesc.height = 3.0f;
    pyramidDesc.rotationRadians = 2.0f;


    Engine::GFX::sMeshData cubeData = Engine::GFX::cMeshGenerator::CreateCube(cubeDesc);
    Engine::GFX::sMeshData pyramidData = Engine::GFX::cMeshGenerator::CreatePyramid(pyramidDesc);


    m_cubeMesh = Engine::GFX::CreateMesh(cubeData);
    m_pyramidMesh = Engine::GFX::CreateMesh(pyramidData);


    Engine::GFX::SubmitMesh(m_cubeMesh);
    Engine::GFX::SubmitMesh(m_pyramidMesh);



    m_playerInstance = m_pool.Create();

    m_playerPosition = Engine::Math::cVec3f(0.0f, 0.0f, 0.0f);


    m_playerInstance->worldMatrix =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };


    m_playerInstance->color =
    {
        1.0f,
        0.0f,
        0.0f,
        1.0f
    };


    m_meshInstances[m_cubeMesh].push_back(m_playerInstance);



    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());


    std::uniform_real_distribution<float> positionDistribution(-20.0f, 20.0f);
    std::uniform_real_distribution<float> scaleDistribution(0.3f, 2.0f);
    std::uniform_real_distribution<float> colorDistribution(0.0f, 1.0f);
    std::uniform_int_distribution<int> meshDistribution(0, 1);


    constexpr int instanceCount = 200;


    for (int index = 0; index < instanceCount; ++index)
    {
        Engine::GFX::sInstanceData* pInstance = m_pool.Create();

        if (pInstance == nullptr)
        {
            std::cerr << "Could not create instance.\n";
            return;
        }


        float x = positionDistribution(randomGenerator);
        float y = positionDistribution(randomGenerator) * 0.2f;
        float z = positionDistribution(randomGenerator);

        float scale = scaleDistribution(randomGenerator);


        pInstance->worldMatrix =
        {
            scale, 0.0f, 0.0f, 0.0f,
            0.0f, scale, 0.0f, 0.0f,
            0.0f, 0.0f, scale, 0.0f,
            x,     y,     z,   1.0f
        };


        pInstance->color =
        {
            colorDistribution(randomGenerator),
            colorDistribution(randomGenerator),
            colorDistribution(randomGenerator),
            1.0f
        };


        if (meshDistribution(randomGenerator) == 0)
        {
            m_meshInstances[m_cubeMesh].push_back(pInstance);
        }
        else
        {
            m_meshInstances[m_pyramidMesh].push_back(pInstance);
        }
    }


    RebuildInstanceList();


    std::cout << "Created " << m_instances.size() << " random instances.\n";
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::OnUpdate(float _deltaTime) 
{
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


    UpdateThirdPersonCamera();


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


    m_playerInstance->worldMatrix =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        m_playerPosition.x(),
        m_playerPosition.y(),
        m_playerPosition.z(),
        1.0f
    };
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
