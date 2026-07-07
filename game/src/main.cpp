#include "application.h"

#include "container/pool.h"

#include "graphics/meshGenerator.h"
#include "graphics/instanceData.h"

#include <iostream>
#include <stdexcept>

constexpr int c_instancesPerPage = 800;

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

        void OnUpdate(float deltaTime) override
        {
            // Wird jeden Frame aufgerufen
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
            // Wird am Ende aufgerufen
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
