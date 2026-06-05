#pragma once 
#include <vulkan/vulkan.h>
#include <vector>
#include "CommandBufferBlueprint.h"
#include "Fence.h"
#include "Semaphore.h"

struct OneShotCommandBuffers  {
	std::vector<VkCommandBuffer> buffers; // Gets created from blueprints. Each blueprint represent seperate rendering phase(0 -> Game, 1 -> Editor Objects, 3 -> ImGUI, etc)
	std::vector<recordFunc> commandsToRecord; 
	std::vector<bool> recorded; 
};
