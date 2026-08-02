#include "GPUSceneManager.h"
#include "Utilities.h"

namespace Graphics
{
void GPUSceneManager::changeGPUScene(gpuSceneFunc defineLayouts_, gpuSceneFunc defineResources_, gpuSceneFunc definePasses_, gpuSceneFunc destroy_)
{
	this->destroy(); 
	
	this->defineLayouts = defineLayouts_; 
	this->defineResources = defineResources_; 
	this->definePasses = definePasses_; 
	this->destroy = destroy_; 
	this->changed = true; 
}
}
