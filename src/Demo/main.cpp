#include <iostream>
#include "Core.h"
#include "Triangle.h"

int main()
{
	try
    {
	    Graphics::Triangle::setGPUSceneToTriangle();
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
