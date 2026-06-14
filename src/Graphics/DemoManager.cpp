#include "DemoManager.h"

uint32_t DemoManager::addCmdBufferBlueprint(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
{
	if(poolType_ == FRAME)
	{
		switch (queueFamilyEnum_)
		{
			case GRAPHICS: this->oneShotCmdBufferBlueprints.graphics.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotCmdBufferBlueprints.graphics.size() - 1;
			case TRANSFER: this->oneShotCmdBufferBlueprints.transfer.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotCmdBufferBlueprints.graphics.size() - 1;
			case COMPUTE: this->oneShotCmdBufferBlueprints.compute.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotCmdBufferBlueprints.graphics.size() - 1;
		}
	}
	else 
	{
		switch (queueFamilyEnum_)
		{
			case GRAPHICS: this->frameCmdBufferBlueprints.graphics.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotCmdBufferBlueprints.graphics.size() - 1;
			case TRANSFER: this->frameCmdBufferBlueprints.transfer.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotCmdBufferBlueprints.graphics.size() - 1;
			case COMPUTE: this->frameCmdBufferBlueprints.compute.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotCmdBufferBlueprints.graphics.size() - 1;
		}
	}
}

