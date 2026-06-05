#include "RenderFlow.h"

void DemoManager::addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
{
	if(poolType_ == FRAME)
	{
		switch (queueFamilyEnum_)
		{
			case GRAPHICS: this->oneShotCmdBufferBlueprints.graphics.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
			case TRANSFER: this->oneShotCmdBufferBlueprints.transfer.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
			case COMPUTE: this->oneShotCmdBufferBlueprints.compute.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		}
	}
	else 
	{
		switch (queueFamilyEnum_)
		{
			case GRAPHICS: this->frameCmdBufferBlueprints.graphics.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
			case TRANSFER: this->frameCmdBufferBlueprints.transfer.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
			case COMPUTE: this->frameCmdBufferBlueprints.compute.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		}
	}
}

