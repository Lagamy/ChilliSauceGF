#include "Renderer.h"
#include "CommandBufferBlueprint.h"
#include "HelperGlobals.h"
#include "Triangle.h"
#include "Globals.h"
#include <vulkan/vulkan_core.h>

void Renderer::setup() 
{
	// Vulkan setup 
	this->instance.setup();
	//	createDebugMessenger();
	this->surface.create(); // Device needs to know - what surface will be used, so I could check if device supports it. 
	this->mainDevice.setup();
	this->swapchain.create();
	

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
}

void Renderer::run() 
{
	this->currentFrameAtFlight = std::max(this->currentFrameAtFlight + 1, this->framesAtFlightCount);  
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

Renderer::~Renderer() 
{
	this->shutdown(); 
}
