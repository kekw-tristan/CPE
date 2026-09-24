#include "game.h"
#include "world/forestAtmosphere.h"

#include <string_view>

int main(int _argc, char** _argv)
{
    try
    {
        Engine::sAppConfig config = {1280, 720, "Game", false};

        config.backgroundColor          = { World::c_fogRed, World::c_fogGreen, World::c_fogBlue, 1.0f };
        config.environment.groundColor  = { World::c_groundBounceRed, World::c_groundBounceGreen, World::c_groundBounceBlue };
        config.environment.horizonColor = { World::c_fogRed, World::c_fogGreen, World::c_fogBlue };
        config.environment.zenithColor  = { World::c_skyRed, World::c_skyGreen, World::c_skyBlue };
        config.environment.keyDirection = { World::c_moonDirectionX, World::c_moonDirectionY, World::c_moonDirectionZ };
        config.environment.keyRadiance  = { World::c_moonRed * World::c_moonRadiance, World::c_moonGreen * World::c_moonRadiance, World::c_moonBlue * World::c_moonRadiance };

        cGame game(config);
#if defined(GAME_DEBUG)
        for (int i = 1; i < _argc; ++i)
        {
            if (std::string_view(_argv[i]) == "--mushroom-preview")
                game.EnableMushroomPreview();
        }
#endif
        game.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
