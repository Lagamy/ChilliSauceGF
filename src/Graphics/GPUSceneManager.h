// Manages demo setup and connects it to the renderer.
#pragma once 

#include "Utilities.h"
#include "PassesPack.h"
#include "Triangle.h"
#include <vector>
#include <thread>

namespace Graphics
{

// TODO: Make it dynamically editable. () 

using gpuSceneFunc = std::function<void()>;
struct GPUSceneManager 
{
	void changeGPUScene(gpuSceneFunc defineLayouts_, gpuSceneFunc defineResources_, gpuSceneFunc definePasses_, gpuSceneFunc syncLayouts_, gpuSceneFunc syncResources_, gpuSceneFunc syncPasses_, gpuSceneFunc destroy_); // If you are making an engine -> this one will be usefull 
	void initGPUScene(); 
	gpuSceneFunc defineLayouts; // Vertex, DS, etc layouts 
	gpuSceneFunc defineResources; // Mesh/Textures
	gpuSceneFunc definePasses; 

	gpuSceneFunc syncLayouts; // add/remove Layouts. Usually during scene transition  
	gpuSceneFunc syncResources; // add new Meshes/Textures or edit existing once in the GPU
	gpuSceneFunc syncPasses; // enables or disbles existing passes
	gpuSceneFunc destroy = []{};

	bool changed; 
};
}
