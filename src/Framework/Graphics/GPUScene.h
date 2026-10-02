// Manages demo setup and connects it to the renderer.
#pragma once 

#include "Utilities.h"
#include "PassesPack.h"
#include "Triangle.h"
#include <vector>
#include <thread>

namespace Graphics
{

using gpuSceneFunc = void(*)();
struct GPUSceneManager 
{
	void changeGPUScene(gpuSceneFunc defineLayouts_, gpuSceneFunc defineResources_, gpuSceneFunc definePasses_, gpuSceneFunc updateLayouts_, gpuSceneFunc updateResources_, gpuSceneFunc updatePasses_, gpuSceneFunc destroy_); // If you are making an engine -> this one will be usefull 
	void initGPUScene(); 
	gpuSceneFunc defineLayouts; // Vertex, DS, etc layouts 
	gpuSceneFunc defineResources; // Mesh/Textures
	gpuSceneFunc definePasses; 

	gpuSceneFunc updateLayouts; // add/remove Layouts. Usually during scene transition  
	gpuSceneFunc updateResources; // add new Meshes/Textures or edit existing once in the GPU
	gpuSceneFunc updatePasses; // enables or disbles existing passes
	gpuSceneFunc destroy = EmptyFunction;

	bool changed; 
};
}
