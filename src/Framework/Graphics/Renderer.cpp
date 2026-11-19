#include "Renderer.h"
#include "Api.h"
#include "Globals.h"
#include <format>
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

void Renderer::setup() 
{
	this->projectManager.setupEnvironment(); 
	// Vulkan setup 
	this->instance.setup();
	//	createDebugMessenger();
	this->surface.create(); // I need to know - what surface will be used, so I could check if device supports it. 
	this->mainDevice.setup();
	createDeviceObjects();
}

void Renderer::createDeviceObjects()
{
	this->swapchain.create();
	this->framesResources.resize(this->framesAtFlightCount);
	for (uint32_t i = 0; i < this->framesAtFlightCount; i++)
	{
		this->framesResources[i].setup(i);
	}
	this->memoryManager.setup();
	

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

	// GPU Scene Description setup 
	this->gpuSceneManager.defineLayouts();
	this->gpuSceneManager.defineResources(); 
	this->gpuSceneManager.definePasses();
	this->gpuSceneManager.changed = false;

	// Descriptor memory has to be added before the static heaps are allocated
	this->descriptorsManager.allocateAllDescriptorsMemory();

	// Send static uploads to the GPU
	this->memoryManager.staticAllocator.allocateAndUpload();

	// Descriptor sets point at the buffers that exist after the static allocation. Pipeline layouts need the set layouts, so those go before the pipelines
	this->descriptorsManager.createAll();

	// Create Pipelines, CmdBuffers
	createAllPipelines(); 
	this->oneShotCommandPools.create();
	for (uint32_t i = 0; i < this->framesAtFlightCount; i++)
	{
		this->framesResources[i].createCmdPools();
	}
}

void Renderer::draw() 
{
	if(!Globals::resizing)
	{
		// GPU Scene Description setup 
		if(this->gpuSceneManager.changed)
		{
			this->gpuSceneManager.defineLayouts();
			this->gpuSceneManager.defineResources(); 
			this->gpuSceneManager.definePasses();
			this->gpuSceneManager.changed = false;

			this->descriptorsManager.allocateAllDescriptorsMemory();
			this->descriptorsManager.createAll();
			createAllPipelines();
		}

		this->memoryManager.resolvePendingOperations();
		this->gpuSceneManager.updateLayouts(); 
		this->gpuSceneManager.updateResources();
		this->gpuSceneManager.updatePasses(); 

		this->passesGraph.waitForFrame(this->currentFrame);
		/* After the fence wait and before the shadows are copied: a load writes the shadow of a per-frame descriptor, which syncCopiesToFrame then copies into this frame's copy. */
		this->descriptorsManager.updateLoading();
		this->passesGraph.releaseHeldPasses();
		this->memoryManager.syncCopiesToFrame(this->currentFrame);

		vkAcquireNextImageKHR(
			this->mainDevice.logicalDevice, this->swapchain.get(), std::numeric_limits<uint64_t>::max(), 
			getSemaphore(this->framesResources[currentFrame].imageAcquiredSemaphoreId), VK_NULL_HANDLE, &imageIndex
		);
		// Rerecord enabled cmdBuffers(purely CPU sided. GPU just pulls them when needed)
		this->oneShotCommandPools.rerecordEnabledCmdBuffers();
		getCurrentFrameResources().frameCmdPools.rerecordEnabledCmdBuffers();

	
		// Compile passesGraph
		this->passesGraph.compileIfDirty(); 
		// Resolve dynamic sync and Submit graph to queues
 		this->passesGraph.resolveSync_SubmitToGPU();

		// if Data was 
		this->descriptorsManager.lockBoundDescriptors();
		this->descriptorsManager.waitForLoads();
		
		presentToScreen(); 
		this->currentFrame = (this->currentFrame + 1) % this->framesAtFlightCount; 
	}
}

void Renderer::destroyDeviceObjects()
{
	/* Vulkan only allows destroying an object once no submitted work uses it, so the device goes idle first. */
	vkDeviceWaitIdle(this->mainDevice.logicalDevice);
	this->syncManager.destroy();
	for (auto& rFrameResources : this->framesResources)
	{
		rFrameResources.destroy(); // CMDPools init for each queue family per frame in flight  
	}
	this->oneShotCommandPools.destroy();
	this->pipelinesManager.destroyAllPipelines(); 
	this->descriptorsManager.destroy();
	this->renderPasses.clear();
	this->swapchain.destroyFramebuffers(); 
	this->presentationRenderPass.destroy(); 
	this->shadersManager.destroy(); 
	this->memoryManager.destroy(); 
	this->swapchain.destroy(); 
}

/*
	Destroying the Vulkan objects leaves the CPU side of the managers behind (entries, passes, layouts, ids). Rebuilding on top of it would add everything twice, so it is reset here.
	Pools are cleared, not recreated, so their generations keep growing and an id from before the reset stays invalid.
	Runs after destroyDeviceObjects and before the device is destroyed: clearing the image pools still calls vkDestroyImage.
*/
void Renderer::resetDeviceState()
{
	this->passesGraph = PassesGraph();
	this->commandLists.clear();
	this->memoryManager = MemoryManager();
	this->descriptorsManager.boundDescriptors.clear();
	this->descriptorsManager.loadsToWaitFor.clear();
	for (auto& rLayouts : this->reflectionSystem.layoutsPerBufferType)
	{
		rLayouts.clear();
	}
	this->resourcesManager.meshes.clear();
	this->resourcesManager.textures.clear();
	this->imagesManager.cubemaps.clear();
	this->imagesManager.volumetrics.clear();
	this->imagesManager.textures.clear();
	this->imagesManager.renderTargets.clear();
	this->presentationRenderPass = RenderPass();
	this->swapchain.renderTargets.clear();
	this->swapchain.framebuffers.clear();
	this->framesResources.clear();
	this->oneShotCommandPools = CmdPoolsPack<OneShotCommandPool>();
	this->currentFrame = 0;
	this->recordingPipelineId = UninitializedPoolId;
	this->recordingSetIds.clear();
}

/*
	Rebuilds the renderer on another suitable GPU. The instance, surface and window stay; everything that depends on the device is destroyed and created again, and the scene comes back through the define* callbacks.
	Ids the application kept from before (meshes, descriptor sets, passes) are invalid afterwards unless define* recreates them.
	Waits for the GPU, so don't call it while recording or inside draw.
*/
void Renderer::switchDevice(uint32_t id_)
{
	if (id_ >= this->mainDevice.suitableDevices.size())
	{
		throw std::runtime_error(std::format("Renderer: can't switch to device {}, there are {} suitable devices.", id_, this->mainDevice.suitableDevices.size()));
	}
	if (id_ == this->mainDevice.mainDeviceId)
	{
		return;
	}

	this->gpuSceneManager.destroy();
	destroyDeviceObjects();
	resetDeviceState();
	this->mainDevice.destroy();
	this->mainDevice.selectDevice(id_);
	this->mainDevice.createLogicalDevice();
	this->projectManager.setupEnvironment();
	createDeviceObjects();
}

void Renderer::shutdown() 
{
	destroyDeviceObjects();
	this->mainDevice.destroy(); 
	this->surface.destroy(); 
	this->instance.destroy();
}
}
