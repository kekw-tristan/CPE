#include "game.h"

int main()
{
    try
    {
        Engine::sAppConfig config = {1280, 720, "Game"};

        cGame game(config);
        game.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}