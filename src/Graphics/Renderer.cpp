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
	this->memoryManager.create();
	

	// Configure RenderPass
	this->presentationRenderPass.addColorAttachment(getSwapchain().imageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,  VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR); // STORE_OP_DONT_CARE - means we dont care what will happen to the attachment after reading it
	SubPassDescriptionInfo subpassDescription = {}; 
	subpassDescription.pRenderPass = &this->presentationRenderPass; 
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
	subpassDescription.colorAttachmentsToUseIds = {0};

	SubPassLayoutTransitionInfo subpassLayoutTransition = {};
	subpassLayoutTransition.stageMaskFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
	subpassLayoutTransition.accessMaskFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassLayoutTransition.dependencyFlags = 0; 
	this->presentationRenderPass.addSubpass(subpassDescription, subpassLayoutTransition); 
	
	// Create Swapchain related resources
	this->presentationRenderPass.create();
	this->swapchain.createFramebuffers(this->presentationRenderPass);
	this->framesResources.resize(this->framesAtFlightCount);

	// Demo setup 
	this->demoManager.defineDemo();

	// Send static uploads to the GPU
	this->memoryManager.staticAllocator.allocateAndUpload();

	// Create Pipelines, CmdBuffers and Synchronisation
	createAllPipelines(); 
	this->oneShotCommandPools.create();
	for (uint32_t i = 0; i < this->framesAtFlightCount; i++)
	{
		this->framesResources[i].setup(i);
	}
}

void Renderer::draw() 
{
	if(!Globals::resizing)
	{
		this->memoryManager.checkUploadsStatus(); 
		VkFence* pCurrentFrameAvailable = &getFence(this->framesResources[this->currentFrame].frameAvailableFenceId);
		vkWaitForFences(this->mainDevice.logicalDevice, 1, pCurrentFrameAvailable, VK_TRUE, std::numeric_limits<uint64_t>::max()); // wait for frame available fence signal
		vkResetFences(this->mainDevice.logicalDevice, 1, pCurrentFrameAvailable); // unsignal fence
																
		vkAcquireNextImageKHR(
			this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
			getSemaphore(this->framesResources[currentFrame].imageAcquiredSemaphoreId), VK_NULL_HANDLE, &imageIndex
		);
		// Rerecord enabled cmdBuffers
		this->oneShotCommandPools.rerecordEnabledCmdBuffers();
		getCurrentFrameResources().frameCmdPools.rerecordEnabledCmdBuffers();

		// Compile passesGraph 
		this->passesGraph.compileIfDirty(); 

		// Submit graph to queues
		this->passesGraph.submitToGPU();
		
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
	this->pipelinesManager.destroyAllPipelines(); 
	this->renderPasses.clear();
	this->swapchain.destroyFramebuffers(); 
	this->presentationRenderPass.destroy(); 
	this->shadersManager.destroy(); 
	this->memoryManager.destroy(); 
	this->swapchain.destroy(); 
	this->mainDevice.destroy(); 
	this->surface.destroy(); 
	this->instance.destroy();
}
}
