#pragma once
#include "Task.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics
{
struct CmdBufsInFramePasses { 
	std::vector<VkCommandBuffer> buffers;
	std::vector<CmdBufferFunc> commandsToRecord;
};
}
