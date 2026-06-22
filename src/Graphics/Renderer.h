#pragma once 
#include "GPUMemoryManager.h"
#include "DemoManager.h"
#include "Pool.h"
#include "RenderPass.h" 
#include "Instance.h"
#include "Device.h"
#include "Surface.h"
#include "Swapchain.h"
#include "FrameResources.h"
#include "GraphicsPipeline.h"
#include "Utilities.h"
#include "SyncManager.h"
#include <limits>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <vector>

namespace Graphics 
{
struct Renderer {
	Instance instance; // Connects this programm and the Vulkan library and encapsulates all Vulkan actions in that instance/app. (So different programs using Vulkan aren’t fighting)
	Device mainDevice;
	Surface surface;
	Swapchain swapchain;
	GPUMemoryManager gpuMemoryManager;
	SyncManager syncManager; 

	// I have only one RenderTarget and only one material type(PBR). So having single 
	RenderPass renderpass;
	GraphicsPipeline graphicsPipeline; 

	DemoManager demoManager;
	std::vector<FrameResources> framesResources; // Initialized by defined RenderFlows 
	CmdPoolsPack<OneShotCommandPool> oneShotCommandPools; 

	uint32_t framesAtFlightCount = 3;
	uint32_t currentFrameAtFlight; 
	uint32_t queueFamiliesCount = 3; 


	PoolId imageAvailableSemaphoreId; 
	void setup();
	void draw(); 
	void shutdown();

	

	void initRenderFlow(); // init all command buffer blueprints

	~Renderer();
	Renderer();
};
}
