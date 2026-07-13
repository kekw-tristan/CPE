#include "application.h"

#include "container/pool.h"

#include "graphics/camera.h"
#include "graphics/meshGenerator.h"
#include "graphics/instanceData.h"

#include <iostream>
#include <stdexcept>

constexpr int c_instancesPerPage = 800;

constexpr int c_shiftKey        = 340;
constexpr int c_downArrowKey    = 264; 
constexpr int c_upArrowKey      = 265; 
constexpr int c_leftArrowKey    = 263; 
constexpr int c_rightArrowKey   = 262; 

class cGame : public Engine::cApplication
{

    public:

        cGame(Engine::sAppConfig& _rAppConfig)
            : Engine::cApplication(_rAppConfig)
        {
        }

    protected:

        void OnInit() override
        {
            Engine::GFX::sCubeDesc cubeDesc;

            cubeDesc.width  = 1.0f;
            cubeDesc.height = 1.0f;
            cubeDesc.depth  = 1.0f;
            cubeDesc.color = { 1.0f, 1.0f, 1.0f, 1.0f };

            Engine::GFX::sMeshData cubeData =
                Engine::GFX::cMeshGenerator::CreateCube(cubeDesc);

            m_cubeMesh = Engine::GFX::CreateMesh(cubeData);

            std::cout << "Mesh: " << cubeData.pDebugName << '\n';

            Engine::GFX::SubmitMesh(m_cubeMesh);

            // Es werden nur noch zwei Instanzen benötigt.
            m_instances.reserve(2);

            Engine::GFX::sInstanceData* pFirstCube = m_pool.Create();

            if (pFirstCube == nullptr)
            {
                std::cerr << "Could not create first cube instance.\n";
                return;
            }

            pFirstCube->worldMatrix =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
               -1.5f, 0.0f, 0.0f, 1.0f
            };

            pFirstCube->color =
            {
                0.0f,
                0.8f,
                0.2f,
                1.0f
            };

            m_instances.push_back(pFirstCube);

            Engine::GFX::sInstanceData* pSecondCube = m_pool.Create();

            if (pSecondCube == nullptr)
            {
                std::cerr << "Could not create second cube instance.\n";
                return;
            }

            pSecondCube->worldMatrix =
            {
                1.5f, 0.0f, 0.0f, 0.0f,  // X-Skalierung
                0.0f, 0.75f, 0.0f, 0.0f, // Y-Skalierung
                0.0f, 0.0f, 0.5f, 0.0f,  // Z-Skalierung
                1.5f, 0.0f, 0.0f, 1.0f   // Position
            };

            pSecondCube->color =
            {
                1.0f,
                0.3f,
                0.0f,
                1.0f
            };

            m_instances.push_back(pSecondCube);

            std::cout
                << "Created "
                << m_instances.size()
                << " cube instances.\n";
        }

        void OnUpdate(float _deltaTime) override
        {
            using namespace Engine::GFX;
            using namespace Engine::Platform;

            float moveSpeed = 3.0f;

            if (IsKeyDown(c_shiftKey))
            {
                moveSpeed = 8.0f;
            }

            const float moveAmount = moveSpeed * _deltaTime;

            cCamera& rCamera = GetCamera();

            if (IsKeyDown('W'))
            {
                rCamera.MoveForward(moveAmount);
            }

            if (IsKeyDown('S'))
            {
                rCamera.MoveForward(-moveAmount);
            }

            if (IsKeyDown('A'))
            {
                rCamera.MoveRight(-moveAmount);
            }

            if (IsKeyDown('D'))
            {
                rCamera.MoveRight(moveAmount);
            }


            if (IsKeyDown(c_downArrowKey))
            {
                rCamera.AddPitch(-100 * _deltaTime);
            };

            if (IsKeyDown(c_upArrowKey))
            {
                rCamera.AddPitch(100 * _deltaTime);
            };

            if (IsKeyDown(c_leftArrowKey))
            {
                rCamera.AddYaw(-100 * _deltaTime);
            };

            if (IsKeyDown(c_rightArrowKey))
            {
                rCamera.AddYaw(100 * _deltaTime);
            };

            Engine::GFX::UpdateInstanceBuffer(m_instances);
        }

        void OnDraw() override
        {
            //for (auto* pInstanceData : m_instances)
            //{
            //    Engine::GFX::Draw(m_cubeMesh, pInstanceData->worldMatrix);
            //}

            Engine::GFX::DrawMeshIntances(m_cubeMesh, m_instances);
        }

        void OnShutdown() override
        {
            for (auto* pInstance : m_instances)
            {
                m_pool.Destroy(pInstance);
            }
        }

        private:

            Engine::GFX::MeshHandle m_cubeMesh;
            Engine::Container::cPool<Engine::GFX::sInstanceData, c_instancesPerPage> m_pool;
            std::vector<Engine::GFX::sInstanceData*> m_instances;
};

int main()
{
    try
    {
        Engine::sAppConfig config = { 1280, 720, "Game" };

        cGame game(config);
        game.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }    
}
