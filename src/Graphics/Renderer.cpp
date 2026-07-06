#include "Renderer.h"
#include "Api.h"
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
	this->gpuMemoryManager.staticAllocator.allocate(); 

	// Renderpass and Graphics pipeline are defined defined by Scene
	this->renderpass.create();
	this->swapchain.createFramebuffers(this->renderpass);
	this->graphicsPipeline.create(this->renderpass, 0);

	this->framesResources.resize(this->framesAtFlightCount);

	// Create CmdBuffers and Synchronisation
	this->oneShotCommandPools.create();
	for (uint32_t i = 0; i < this->framesAtFlightCount; i++)
	{
		this->framesResources[i].setup(i);
	}
	this->gpuMemoryManager.submitStaticUploadCmds(); // Upload all preloaded with scene / static assets to the GPU
}

void Renderer::draw() 
{

	VkFence* pCurrentFrameAvailable = &this->syncManager.getFence(this->framesResources[this->currentFrame].frameAvailableFenceId).vkHandle;

	vkWaitForFences(this->mainDevice.logicalDevice, 1, pCurrentFrameAvailable, VK_TRUE, std::numeric_limits<uint64_t>::max()); // wait for frame available fence signal
	vkResetFences(this->mainDevice.logicalDevice, 1, pCurrentFrameAvailable); // unsignal fence
																
	vkAcquireNextImageKHR(
		this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
		this->syncManager.getSemaphore(this->framesResources[currentFrame].imageAvailableSemaphoreId).vkHandle, VK_NULL_HANDLE, &imageIndex
	);
	// NOTE: you need to pass this fence with frame submit in your demo code. Otherwise - nothing will signal this fence and app will freeze
	resetCurrentFrameCmdPools();
	recordCurrentFrameCmdPools(); 

	this->demoManager.submitToGPU();
	this->currentFrame = (this->currentFrame + 1) % this->framesAtFlightCount; 
}

void Renderer::shutdown() 
{
	vkDeviceWaitIdle(this->mainDevice.logicalDevice);
	this->syncManager.destroy();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.frameCmdPools.destroy(); // CMDPools init for each queue family per frame in flight  
	}
	this->oneShotCommandPools.destroy();
	this->graphicsPipeline.destroy(); 
	this->swapchain.destroyFramebuffers(); 
	this->renderpass.destroy(); 
	this->shadersManager.destroy(); 
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
}
