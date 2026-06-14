#include "Renderer.h"
#include "CommandBufferBlueprint.h"
#include "HelperGlobals.h"
#include "Triangle.h"
#include "Globals.h"
#include "Utilities.h"
#include <limits>
#include <vulkan/vulkan_core.h>

void Renderer::setup() 
{
	// Vulkan setup 
	this->instance.setup();
	//	createDebugMessenger();
	this->surface.create(); // I need to know - what surface will be used, so I could check if device supports it. 
	this->mainDevice.setup();
	this->swapchain.create();
	this->gpuMemoryManager.create(); 

	// Scene/Renderer setup 
	demoManager.loadDemo();
	

	// Renderpass and Graphics pipeline are defined defined by Scene
	this->renderpass.create();
	this->swapchain.createFramebuffers(this->renderpass);
	this->graphicsPipeline.create(this->renderpass, 1);

	this->framesResources.resize(this->framesAtFlightCount);

	// Create CmdBuffers and Synchronisation
	this->oneShotCommandPools.create();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.frameCmdPools.create(); // CMDPools init for each queue family per frame in flight  
		rFrameResources.frameFinished.create();
	}
	this->imageAvailable.create();
}

void Renderer::draw() 
{
	vkAcquireNextImageKHR(
		this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
		this->imageAvailable.get(), VK_NULL_HANDLE, &currentFrameAtFlight
	);
	
	demoManager.submitToGPU();	
}

void Renderer::shutdown() 
{
	this->imageAvailable.destroy();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.frameFinished.destroy();
		rFrameResources.frameCmdPools.destroy(); // CMDPools init for each queue family per frame in flight  
	}
	this->oneShotCommandPools.destroy();
	this->graphicsPipeline.destroy(); 
	this->swapchain.destroyFramebuffers(); 
	this->renderpass.destroy(); 
	this->gpuMemoryManager.destroy(); 
	this->swapchain.destroy(); 
	this->mainDevice.destroy(); 
	this->surface.destroy(); 
	this->instance.destroy();
}

VkCommandBuffer& Renderer::getCommandBuffer(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_, uint32_t id_)
{
	if(poolType_ == FRAME)
	{
		return this->framesResources[this->currentFrameAtFlight].frameCmdPools.getPoolByQueue(queueFamily_).commandBuffers.buffers[id_];
	}
	else
	{
		return this->oneShotCommandPools.getPoolByQueue(queueFamily_).commandBuffers.buffers[id_];
	}
}

VkCommandPool& Renderer::getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_)
{
	if(poolType_ == FRAME)
	{
		return this->framesResources[this->currentFrameAtFlight].frameCmdPools.getPoolByQueue(queueFamily_).get();
	}
	else
	{
		return this->oneShotCommandPools.getPoolByQueue(queueFamily_).get();
	}
}

void Renderer::recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_)
{
	this->oneShotCommandPools.pools[queueFamily_].recordCmdBuffer(id_);
}

void Renderer::recordFrameCmdPools()
{
	for(auto& rFrameCmdPool : this->framesResources[this->currentFrameAtFlight].frameCmdPools.pools)
	{
		rFrameCmdPool.recordCmdBuffers();
	}
}

void Renderer::resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_)
{

	this->oneShotCommandPools.pools[queueFamily_].resetCmdBuffer(id_);
}

void Renderer::resetFrameCmdPools()
{
	for(auto& rFrameCmdPool : this->framesResources[this->currentFrameAtFlight].frameCmdPools.pools)
	{
		rFrameCmdPool.resetCmdPool(this->framesResources[this->currentFrameAtFlight].frameFinished);
	}
}

VkCommandPool& Renderer::getCommandPool(QueueFamilyEnum queueFamily_, CommandPoolTypeEnum poolType_)
{
	if(poolType_ == ONESHOT) 
	{
		this->oneShotCommandPools.getPoolByQueue(queueFamily_);
	}
	else 
	{
		this->framesResources[this->currentFrameAtFlight].frameCmdPools.getPoolByQueue(queueFamily_);
	} 
}

Renderer::~Renderer() 
{
	this->shutdown(); 
}
