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
	

	// Demo setup 
	this->demoManager.defineDemo();

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
	
	// Send static uploads to the GPU
	this->gpuMemoryManager.staticAllocator.allocate(); 

	// Create GPU resources
	this->presentationRenderPass.create();
	this->swapchain.createFramebuffers(this->presentationRenderPass);
	this->framesResources.resize(this->framesAtFlightCount);
	createAllPipelines();

	// Create CmdBuffers and Synchronisation
	this->oneShotCommandPools.create();
	for (uint32_t i = 0; i < this->framesAtFlightCount; i++)
	{
		this->framesResources[i].setup(i);
	}
	this->gpuMemoryManager.submitStaticUploads(); // Upload all preloaded with scene / static assets to the GPU
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
		// Rerecord enabled cmdBuffers
		this->oneShotCommandPools.rerecordEnabledCmdBuffers();
		getCurrentFrameResources().frameCmdPools.rerecordEnabledCmdBuffers();
		
		// Submit Passes to queues
		submitPassesToQueues();
		
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
	this->gpuPipelinesManager.destroyAllPipelines(); 
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
