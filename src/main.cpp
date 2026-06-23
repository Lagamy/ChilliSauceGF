#include "Core.h"

int main()
{
	printf("Print test"); 
	Core core; 
	core.setup();
	core.run();
	core.shutdown();
}
