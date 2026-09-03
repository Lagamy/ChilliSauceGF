#include "GPUScene.h"
#include "Utilities.h"

namespace Graphics
{
void GPUScene::changeGPUScene(gpuSceneFunc defineLayouts_, gpuSceneFunc defineResources_, gpuSceneFunc definePasses_, gpuSceneFunc syncLayouts_, gpuSceneFunc syncResources_, gpuSceneFunc syncPasses_, gpuSceneFunc destroy_)
{
	this->destroy(); 
	
	this->defineLayouts = defineLayouts_; 
	this->defineResources = defineResources_; 
	this->definePasses = definePasses_; 
	this->updateLayouts = syncLayouts_; 
	this->updateResources = syncResources_; 
	this->updatePasses = syncPasses_; 
	this->destroy = destroy_; 
	this->changed = true; 
}
}
