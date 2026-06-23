#include "application.h"

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
            // Wird einmal vor der Main Loop aufgerufen
        }

        void OnUpdate(float deltaTime) override
        {
            // Wird jeden Frame aufgerufen
        }

        void OnDraw() override
        {
            // Wird jeden Frame während Rendering aufgerufen
        }

        void OnShutdown() override
        {
            // Wird am Ende aufgerufen
        }
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
