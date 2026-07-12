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

            
            constexpr int c_gridSize = 100;
            constexpr float c_spacing = 2.0f;

            constexpr std::size_t c_instanceCount =
                static_cast<std::size_t>(c_gridSize) *
                c_gridSize *
                c_gridSize;

            m_instances.reserve(c_instanceCount);

            constexpr float c_gridOffset =
                static_cast<float>(c_gridSize - 1) *
                c_spacing *
                0.5f;

            for (int zIndex = 0; zIndex < c_gridSize; ++zIndex)
            {
                for (int yIndex = 0; yIndex < c_gridSize; ++yIndex)
                {
                    for (int xIndex = 0; xIndex < c_gridSize; ++xIndex)
                    {
                        Engine::GFX::sInstanceData* pInstanceData =
                            m_pool.Create();

                        if (pInstanceData == nullptr)
                        {
                            std::cerr
                                << "Instance pool exhausted after "
                                << m_instances.size()
                                << " instances.\n";

                            return;
                        }

                        const float x = static_cast<float>(xIndex) * c_spacing - c_gridOffset;

                        const float y = static_cast<float>(yIndex) * c_spacing - c_gridOffset;

                        const float z = static_cast<float>(zIndex) * c_spacing - c_gridOffset;

                        pInstanceData->worldMatrix =
                        {
                            1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            x,    y,    z,    1.0f
                        };

                        const float xFactor = static_cast<float>(xIndex) / static_cast<float>(c_gridSize - 1);

                        const float yFactor = static_cast<float>(yIndex) / static_cast<float>(c_gridSize - 1);

                        const float zFactor = static_cast<float>(zIndex) / static_cast<float>(c_gridSize - 1);

                        // Unten = 1.0, oben = 0.15
                        const float brightness = 1.0f - yFactor * 0.85f;

                        pInstanceData->color =
                        {
                            xFactor,
                            - yFactor,
                            zFactor,
                            1.0f
                        };

                        m_instances.push_back(pInstanceData);
                    }
                }
            }

            std::cout << "Created " << m_instances.size() << " instances.\n";
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
