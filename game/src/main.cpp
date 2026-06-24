#include "application.h"

#include "graphics/meshGenerator.h"

#include <iostream>
#include <stdexcept>

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
        }

        void OnUpdate(float deltaTime) override
        {
            // Wird jeden Frame aufgerufen
        }

        void OnDraw() override
        {
            
            Engine::GFX::Draw(m_cubeMesh);
        }

        void OnShutdown() override
        {
            // Wird am Ende aufgerufen
        }

        private:

            Engine::GFX::MeshHandle m_cubeMesh;
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
