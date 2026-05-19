#include "CommandBufferBlueprint.h"

void CommandBufferBlueprint::init(recordFunc commandsToRecord_, VkCommandBufferUsageFlags cmdBufferUsageFlags_, bool oneTimeUse_) {
	this->cmdBufferUsageFlags = cmdBufferUsageFlags_; 
	this->commandsToRecord = commandsToRecord_; 
	this->exists = true;
	this->oneTimeUse = oneTimeUse_; 
}


