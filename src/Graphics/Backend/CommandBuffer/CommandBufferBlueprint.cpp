#include "CommandBufferBlueprint.h"

void CommandBufferBlueprint::init(Cmd commandsToRecord_, VkCommandBufferUsageFlags cmdBufferUsageFlags_) {
	this->cmdBufferUsageFlags = cmdBufferUsageFlags_; 
	this->commandsToRecord = commandsToRecord_; 
	this->exists = true;
}


