#pragma once
#include "CommandBufferBlueprint.h"
#include <vulkan/vulkan.h>
#include <vector>

struct FrameCommandBuffers { 
	std::vector<VkCommandBuffer> buffers;
	std::vector<recordFunc> commandsToRecord;
};
