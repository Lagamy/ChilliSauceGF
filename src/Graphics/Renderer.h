#pragma once 
#include <cstdint>
#include <vector>
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

struct Renderer {
	Graphics::Instance instance; // Connects this programm and the Vulkan library and encapsulates all Vulkan actions in that instance/app. (So different programs using Vulkan aren’t fighting)
	Graphics::Device mainDevice;
	Graphics::Surface surface;
	Graphics::Swapchain swapchain;
	Graphics::GPUMemoryManager gpuMemoryManager;
	Graphics::SyncManager syncManager; 

	// I have only one RenderTarget and only one material type(PBR). So having single 
	Graphics::RenderPass renderpass;
	Graphics::GraphicsPipeline graphicsPipeline; 

	Graphics::DemoManager demoManager;
	std::vector<Graphics::FrameResources> framesResources; // Initialized by defined RenderFlows 
	Graphics::CmdPoolsPack<Graphics::OneShotCommandPool> oneShotCommandPools; 

	uint32_t framesAtFlightCount = 3;
	uint32_t currentFrameAtFlight; 
	uint32_t queueFamiliesCount = 3; 


	PoolId imageAvailableSemaphoreId; 
	void setup();
	void draw(); 
	void shutdown();

	

	void initRenderFlow(); // init all command buffer blueprints

	~Renderer();
};

