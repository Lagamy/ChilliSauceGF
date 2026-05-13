#include "Renderer.h"
#include "CMDsRecordFunctions.h"
#include "CommandBufferBlueprint.h"
#include "Engine.h"
#include "HelperGlobals.h"
#include <vulkan/vulkan_core.h>

void Renderer::setup() {

	this->instance.setup();
//	createDebugMessenger();
	this->surface.create(); // Device needs to know - what surface will be used, so I could check if device supports it. 
	this->mainDevice.setup();
	this->swapchain.create();

	this->framesResources.resize(this->framesAtFlightCount);

	for (auto& frameResources : this->framesResources)
	{
		frameResources.init();
	}
}

void Renderer::run() {

}

void Renderer::shutdown() {

	
}

void Renderer::initRenderFlows() { 
	this->gameRenderFlow.initPrimaryCmdBufferBlueprint(GRAPHICS, gameCmdGraphicsCommands);
}

void Renderer::populateCmdPools() { 
	// Game cmd blueprints: 
}

Renderer::~Renderer() {
	this->shutdown(); 
}
