#include "application.h"

#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        Engine::cApplication app;
        app.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    
}
