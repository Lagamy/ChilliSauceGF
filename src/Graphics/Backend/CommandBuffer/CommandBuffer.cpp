#include "CommandBuffer.h"
#include "CommandBufferBlueprint.h"

void CommandBuffer::recordCommands()
{
    // Information about how to begin each buffer
    
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = this->cmdBufferUsageFlags; 

    // Start recording commands to command buffer! 
    VkResult result = vkBeginCommandBuffer(this->vkHandle, &bufferBeginInfo); // result, to check if we can record to it(could be in use, or have "one time submit" flag)
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to start recording a Command Buffer!");
    }
    
	this->commandsToRecord(this->vkHandle);
	
	result = vkEndCommandBuffer(this->vkHandle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to end recording a Command Buffer!");
    }
}

CommandBuffer::CommandBuffer(CommandBufferBlueprint& rBlueprint_, VkCommandBuffer vkHandle_) 
{
	this->commandsToRecord = rBlueprint_.commandsToRecord;
	this->cmdBufferUsageFlags = rBlueprint_.cmdBufferUsageFlags; 
	this->vkHandle = vkHandle_;
	this->oneTimeUse = rBlueprint_.oneTimeUse; 
}

VkCommandBuffer CommandBuffer::get() const {
	return this->vkHandle;
}
