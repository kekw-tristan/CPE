#include "game.h"
#include "world/forestAtmosphere.h"

int main()
{
    try
    {
        Engine::sAppConfig config = {1280, 720, "Game", false};

        config.backgroundColor = { World::c_fogRed, World::c_fogGreen, World::c_fogBlue, 1.0f };

        cGame game(config);
        game.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}