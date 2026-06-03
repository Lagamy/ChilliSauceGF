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
	triangle.load();
	

	// Renderpass and Graphics pipeline are defined defined by Scene
	this->renderpass.create();
	this->swapchain.createFramebuffers(this->renderpass);
	this->graphicsPipeline.create(this->renderpass, 1);

	this->framesResources.resize(this->framesAtFlightCount);

	// Create CmdBuffers and Synchronisation
	this->oneShotCommandPools.create();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.frameCommandPools.create(); // CMDPools init for each queue family per frame in flight  
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
	
	// Transfer 
	

	// Render Image
	VkSubmitInfo submitInfo = {}; 
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 
	submitInfo.waitSemaphoreCount = 1; 
	submitInfo.pWaitSemaphores = &this->imageAvailable.get();
	VkPipelineStageFlags waitStages[] = { 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	// If you will submit multiple CmdBuffers -> they will be executed sequentialy, and Fence would be triggered only after all 3 completed
	submitInfo.pWaitDstStageMask = waitStages; // Till what stage of pipeline app will go, till stoping and waiting for semaphore/s
	submitInfo.commandBufferCount = 1; 
	submitInfo.pCommandBuffers = 
	
}

void Renderer::shutdown() 
{
	this->imageAvailable.destroy();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.frameFinished.destroy();
		rFrameResources.frameCommandPools.destroy(); // CMDPools init for each queue family per frame in flight  
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


void Renderer::recordOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_)
{
	this->oneShotCommandPools.commandPools[queueFamily_].recordCmdBuffer(id_);
}

void Renderer::recordFrameCmdPools()
{
	for(auto& rFrameCmdPool : this->framesResources[this->currentFrameAtFlight].frameCommandPools.commandPools)
	{
		rFrameCmdPool.recordCmdBuffers();
	}
}

void Renderer::resetOneShotCmdBuf(QueueFamilyEnum queueFamily_, uint32_t id_)
{

	this->oneShotCommandPools.commandPools[queueFamily_].resetCmdBuffer(id_);
}

void Renderer::resetFrameCmdPools()
{
	for(auto& rFrameCmdPool : this->framesResources[this->currentFrameAtFlight].frameCommandPools.commandPools)
	{
		rFrameCmdPool.resetCmdPool(this->framesResources[this->currentFrameAtFlight].frameFinished);
	}
}

Renderer::~Renderer() 
{
	this->shutdown(); 
}
