#pragma once
#include "CommandPool.h"

struct FrameResources {
	CommandPool graphicsCommandPool;
	CommandPool transferCommandPool;
	CommandPool computeCommandPool;

	void init(); 
};
