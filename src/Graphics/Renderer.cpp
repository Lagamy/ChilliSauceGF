#include "Renderer.h"
#include "CommandBufferBlueprint.h"
#include "HelperGlobals.h"
#include "Semaphore.h"
#include "Triangle.h"
#include "Globals.h"
#include "Utilities.h"
#include <limits>
#include <vulkan/vulkan_core.h>

// namespace Graphics
// {
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
	this->demoManager.loadDemo();
	

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
		rFrameResources.frameFinishedFenceId = this->syncManager.addFence("Frame Finished");
	}
	this->imageAvailableSemaphoreId = this->syncManager.addSemaphore("Image Available"); 
	this->gpuMemoryManager.submitStaticUploadCmds(); // Upload all preloaded with scene / static assets to the GPU
}

void Renderer::draw() 
{
	vkAcquireNextImageKHR(
		this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
		this->syncManager.getSemaphore(this->imageAvailableSemaphoreId).get(), VK_NULL_HANDLE, &currentFrameAtFlight
	);
	this->resetCurrentFrameCmdPools();
	this->recordCurrentFrameCmdPools(); 

	demoManager.submitToGPU();
}

void Renderer::shutdown() 
{
	this->syncManager.destroy();
	for (auto& rFrameResources : this->framesResources)
	{
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

Renderer::~Renderer() 
{
	this->shutdown(); 
}
// }
