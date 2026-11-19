#pragma once
#include "PassId.h"
#include "Task.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics
{
/* What records one command buffer: the command list of the task, looked up when it is recorded so Task::commandListId can change afterwards. */
struct CmdRecorder
{
	PassId passId;
	uint32_t taskId = UninitializedId;
};

struct CmdBuffers { 
	std::vector<VkCommandBuffer> buffers;
	std::vector<CmdRecorder> commandsToRecord; 
	std::vector<uint32_t> buffersToEnabled; // mirrored 1:1 with buffers vector
	std::vector<uint32_t> enabled; // Depends wether parent pass was enabled or not 
};
}
