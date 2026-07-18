#pragma once 
#include "Task.h"
#include <vulkan/vulkan.h>
#include <vector>


namespace Graphics
{
struct CmdBufsInOneShotPasses  {
	std::vector<VkCommandBuffer> buffers; // Gets created from blueprints. Each blueprint represent seperate rendering phase(0 -> Game, 1 -> Editor Objects, 3 -> ImGUI, etc)
	std::vector<CmdBufferFunc> commandsToRecord;
	std::vector<bool> recorded; 
};
}
