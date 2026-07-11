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

            cubeDesc.width  = 1.f; 
            cubeDesc.height = 1.f; 
            cubeDesc.depth  = 1.f; 
            cubeDesc.color  = { 0.f, 0.2f, 0.1f, 1.f };

            Engine::GFX::sMeshData cubeData = Engine::GFX::cMeshGenerator::CreateCube(cubeDesc);

            m_cubeMesh = Engine::GFX::CreateMesh(cubeData);
            std::cout << "Mesh: " << cubeData.pDebugName << std::endl;

            Engine::GFX::SubmitMesh(m_cubeMesh);

            

            for (int index = 0; index < c_instancesPerPage * 2; ++index)
            {
                Engine::GFX::sInstanceData* pInstanceData = m_pool.Create();

                std::array<float, 16> worldMatrixA =
                {
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    static_cast<float>(index * 2), 0.0f, 0.0f, 1.0f
                };

                pInstanceData->worldMatrix = worldMatrixA; 
                pInstanceData->color = {1.f, 0.f, 0.f, 1.f};

                m_instances.push_back(pInstanceData);
            }

            
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
        }

        void OnDraw() override
        {
            for (auto* pInstanceData : m_instances)
            {
                Engine::GFX::Draw(m_cubeMesh, pInstanceData->worldMatrix);
            }
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
