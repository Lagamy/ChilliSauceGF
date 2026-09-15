#pragma once 
#include "MemoryManager.h"
#include "GPUScene.h"
#include "PipelinesManager.h"
#include "PassesGraph.h"
#include "PoolMap.h"
#include "RenderPass.h" 
#include "Instance.h"
#include "Device.h"
#include "ResourcesManager.h"
#include "ShadersManager.h"
#include "Surface.h"
#include "Swapchain.h"
#include "FrameResources.h"
#include "GraphicsPipeline.h"
#include "Utilities.h"
#include "SyncManager.h"
#include "ShadersManager.h"
#include "ReflectionSystem.h"
#include "ImagesManager.h"
#include <limits>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <vector>


namespace Graphics 
{
struct Renderer {
	Instance instance; // Connects this programm and the Vulkan library and encapsulates all Vulkan actions in that instance/app. (So different programs using Vulkan aren�t fighting)
	Device mainDevice;
	Surface surface;
	Swapchain swapchain;
	MemoryManager memoryManager;
	SyncManager syncManager; 
	ShadersManager shadersManager;
	ImagesManager imagesManager; 
	ResourceManager resourcesManager; 
	PipelinesManager pipelinesManager;
	ReflectionSystem reflectionSystem;
	PassesGraph passesGraph; 

	// I have only one RenderTarget and only one material type(PBR). So having single 
	RenderPass presentationRenderPass;
	PoolMap<RenderPass> renderPasses = ("RenderPasses"); 

	GPUScene gpuSceneManager;
	std::vector<FrameResources> framesResources; // Initialized by defined RenderFlows 
	CmdPoolsPack<OneShotCommandPool> oneShotCommandPools; 

	bool firstFrame = true; 
	uint32_t framesAtFlightCount; // Max amount of frames that can be in the queue at the same time 
	uint32_t currentFrame = 0; 
	uint32_t queueFamiliesCount = 3; 
	uint32_t imageIndex; // Whichever Driver sees as first to become available. Frames are decoupled from images this way. 

	void setup();
	void draw(); 
	void shutdown();

	void initRenderFlow(); // init all command buffer blueprints

};
}
