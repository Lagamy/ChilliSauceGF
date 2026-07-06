#include <iostream>
#include "Core.h"

int main()
{
	try
    {
        Core core;

        core.setup();
        core.run();
        core.shutdown();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what();
        return 1;
    }
	return 0; 
}
