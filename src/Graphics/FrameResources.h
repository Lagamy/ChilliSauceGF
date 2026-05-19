#pragma once
#include "CommandPool.h"
#include "CommandBufferBlueprint.h"

struct FrameResources {
	CommandPool graphicsCommandPool;
	CommandPool transferCommandPool;
	CommandPool computeCommandPool;

	void init(); 
	void resetCMDBuffers(); 
	void destroy(); 
};
