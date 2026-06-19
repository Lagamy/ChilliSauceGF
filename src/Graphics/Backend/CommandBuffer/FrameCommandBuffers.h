#pragma once
#include "CommandBufferBlueprint.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics
{
struct FrameCommandBuffers { 
	std::vector<VkCommandBuffer> buffers;
	std::vector<recordFunc> commandsToRecord;
};
}
