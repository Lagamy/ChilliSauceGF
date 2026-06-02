#include "Renderer.h"
#include "CommandBufferBlueprint.h"
#include "HelperGlobals.h"
#include "Triangle.h"
#include "Globals.h"
#include <limits>
#include <vulkan/vulkan_core.h>

void Renderer::setup() 
{
	// Vulkan setup 
	this->instance.setup();
	//	createDebugMessenger();
	this->surface.create(); // Device needs to know - what surface will be used, so I could check if device supports it. 
	this->mainDevice.setup();
	this->swapchain.create();
	this->gpuMemoryManager.create(); 

	// Scene/Renderer setup 
	triangle.load();
	this->framesResources.resize(this->framesAtFlightCount);

	for (auto& frameResources : this->framesResources)
	{
		frameResources.init(); // CMDPools init for each queue family per frame in flight  
	}

	// Renderpass and Graphics pipeline are defined defined by Scene
	this->renderpass.create();
	this->swapchain.createFramebuffers(this->renderpass);
	this->graphicsPipeline.create(this->renderpass, 1);

	// Create Synchronisation
	this->imageAvailable.create();
}

void Renderer::draw() 
{
	vkAcquireNextImageKHR(
		this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
		this->imageAvailable.get(), VK_NULL_HANDLE, &currentFrameAtFlight
	);
	
	VkSubmitInfo submitInfo = {}; 
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 
	submitInfo.waitSemaphoreCount = 1; 
	submitInfo.pWaitSemaphores = &this->imageAvailable.get();

	VkPipelineStageFlags waitStages[] = { 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages; // Till what stage of pipeline app will go, till stoping and waiting for semaphore/s
	submitInfo.commandBufferCount = 1; 
	submitInfo.pCommandBuffers = &this->framesResources[this->currentFrameAtFlight].graphicsCommandPool.commandBuffers[0].get();
	//this->currentFrameAtFlight = std::max(this->currentFrameAtFlight + 1, this->framesAtFlightCount);  
}

void Renderer::shutdown() 
{
	this->graphicsPipeline.destroy(); 
	this->swapchain.destroyFramebuffers(); 
	this->renderpass.destroy(); 
	for(auto& frameResources : this->framesResources)
	{
		frameResources.destroy(); 
	}
	this->gpuMemoryManager.destroy(); 
	this->swapchain.destroy(); 
	this->mainDevice.destroy(); 
	this->surface.destroy(); 
	this->instance.destroy();
}


void Renderer::populateCmdPools() 
{ 
	// Game cmd blueprints: 
}

void recordOneShotCmdBuf(uint32_t id_);
{

}

void recordFrameCmdPool(); 
{

}

void resetOneShotCmdBuf(uint32_t id_);
{

}

void resetFrameCmdPool();
{

}

Renderer::~Renderer() 
{
	this->shutdown(); 
}
