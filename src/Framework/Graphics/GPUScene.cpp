#include "GPUScene.h"
#include "Utilities.h"

namespace Graphics
{
void GPUSceneManager::changeGPUScene(gpuSceneFunc defineLayouts_, gpuSceneFunc defineResources_, gpuSceneFunc definePasses_, gpuSceneFunc updateLayouts_, gpuSceneFunc updateResources_, gpuSceneFunc updatePasses_, gpuSceneFunc destroy_)
{
	this->destroy(); 
	
	this->defineLayouts = defineLayouts_;
	this->defineResources = defineResources_;
	this->definePasses = definePasses_;
	this->updateLayouts = updateLayouts_;
	this->updateResources = updateResources_;
	this->updatePasses = updatePasses_;
	this->destroy = destroy_;
	this->changed = true;
}
}
