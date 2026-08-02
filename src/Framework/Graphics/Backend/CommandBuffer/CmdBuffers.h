#pragma once
#include "Task.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics
{
struct CmdBuffers { 
	std::vector<VkCommandBuffer> buffers;
	std::vector<CmdBufferFunc> commandsToRecord; 
	std::vector<uint32_t> buffersToEnabled; // mirrored 1:1 with buffers vector
	std::vector<uint32_t> enabled; // Depends wether parent pass was enabled or not 
};
}
