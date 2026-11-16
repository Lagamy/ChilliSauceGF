#include <iostream>
#include "Core.h"
#include "Demo.h"

int main()
{
	try
    {
	    Graphics::Demo::setGPUSceneToDemo();
        Core graphicsFrameworkCore;
        graphicsFrameworkCore.setup();
        graphicsFrameworkCore.run();
        graphicsFrameworkCore.shutdown();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what();
        return 1;
    }
	return 0; 
}
