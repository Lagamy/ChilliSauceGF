// To render properly. 

#pragma once 
#include "GPUMemoryManager.h"
#include "RenderFlow.h"
#include "RenderPass.h" 
#include "Instance.h"
#include "Device.h"
#include "Surface.h"
#include "Swapchain.h"
#include "FrameResources.h"
#include "GraphicsPipeline.h"
#include "Utilities.h"
#include <cstdint>
#include <vector>

struct Renderer {
	Instance instance; // Connects this programm and the Vulkan library and encapsulates all Vulkan actions in that instance/app. (So different programs using Vulkan aren’t fighting)
	Device mainDevice;
	Surface surface;
	Swapchain swapchain;
	GPUMemoryManager gpuMemoryManager;
	Semaphore imageAvailable; 

	// I have only one RenderTarget and only one material type(PBR). So having single 
	RenderPass renderpass;
	GraphicsPipeline graphicsPipeline; 

	RenderFlow renderFlow;
	std::vector<FrameResources> framesResources; // Initialized by defined RenderFlows 
	CommandPoolsPack<OneShotCommandPool> oneShotCommandPools; 

	uint32_t framesAtFlightCount = 3;
	uint32_t currentFrameAtFlight; 
	uint32_t queueFamiliesCount = 3; 

	void setup();
	void draw(); 
	void shutdown();

	

	void initRenderFlow(); // init all command buffer blueprints

	void recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_);
	void recordFrameCmdPools(); 

	void resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_);
	void resetFrameCmdPools();
	
	// void submitCmdBuf(CommandPoolTypeEnum poolType_, uint32_t id_, std::span<VkPipelineStageFlags> waitStages, ); 

	~Renderer();
};
