#include "DemoManager.h"
#include "Utilities.h"

namespace Graphics
{
void DemoManager::defineDemo()
{
	this->defineLayouts(); 
	this->defineResources(); 
	this->definePasses();
}
}
