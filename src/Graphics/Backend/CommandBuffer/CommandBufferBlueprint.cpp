#include "CommandBufferBlueprint.h"

namespace Graphics
{
void CommandBufferBlueprint::init(recordFunc commandsToRecord_, VkCommandBufferUsageFlags cmdBufferUsageFlags_, bool oneTimeUse_) {
	this->cmdBufferUsageFlags = cmdBufferUsageFlags_; 
	this->commandsToRecord = commandsToRecord_; 
}
}
