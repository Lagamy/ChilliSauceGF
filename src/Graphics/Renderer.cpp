#include "Renderer.h"
#include "Api.h"
#include "Globals.h"
#include <limits>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

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
	this->passesManaer.deriveFrameVkSubmitInfos(); 
	this->gpuMemoryManager.staticAllocator.allocate(); 

	// Renderpass and Graphics pipeline are defined defined by Scene
	this->demoManager.createGPUObjects();
	this->swapchain.createFramebuffers(this->presentationRenderPass);

	this->framesResources.resize(this->framesAtFlightCount);

	// Create CmdBuffers and Synchronisation
	this->oneShotCommandPools.create();
	for (uint32_t i = 0; i < this->framesAtFlightCount; i++)
	{
		this->framesResources[i].setup(i);
	}
	this->gpuMemoryManager.submitStaticUploadCMDs(); // Upload all preloaded with scene / static assets to the GPU
}

void Renderer::draw() 
{
	if(!Globals::resizing)
	{
		VkFence* pCurrentFrameAvailable = &this->framesResources[this->currentFrame].frameAvailableFence.get();

		vkWaitForFences(this->mainDevice.logicalDevice, 1, pCurrentFrameAvailable, VK_TRUE, std::numeric_limits<uint64_t>::max()); // wait for frame available fence signal
		vkResetFences(this->mainDevice.logicalDevice, 1, pCurrentFrameAvailable); // unsignal fence
																
		vkAcquireNextImageKHR(
			this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
			this->framesResources[currentFrame].imageAcquireSemaphore.get(), VK_NULL_HANDLE, &imageIndex
		);
		// NOTE: you need to pass this fence with frame submit in your demo code. Otherwise - nothing will signal this fence and app will freeze
		resetCurrentFrameCmdPools();
		recordCurrentFrameCmdPools(); 

		submitToPassesToQueues();
		presentToScreen(); 
		this->currentFrame = (this->currentFrame + 1) % this->framesAtFlightCount; 
	}
}

void Renderer::shutdown() 
{
	vkDeviceWaitIdle(this->mainDevice.logicalDevice);
	this->syncManager.destroy();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.destroy(); // CMDPools init for each queue family per frame in flight  
	}
	this->oneShotCommandPools.destroy();
	this->graphicsPipelines.clear(); 
	this->renderPasses.clear();
	this->swapchain.destroyFramebuffers(); 
	this->presentationRenderPass.destroy(); 
	this->shadersManager.destroy(); 
	this->gpuMemoryManager.destroy(); 
	this->swapchain.destroy(); 
	this->mainDevice.destroy(); 
	this->surface.destroy(); 
	this->instance.destroy();
}
}
