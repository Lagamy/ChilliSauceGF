#include "DemoManager.h"
#include "Utilities.h"

namespace Graphics
{
uint32_t DemoManager::addCmdBufferBlueprint(CmdTypeEnum poolType_, QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
{
	if(poolType_ == ONESHOT)
	{
		switch (queueFamilyEnum_)
		{
			case GRAPHICS: this->oneShotPassesPack.graphics.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotPassesPack.graphics.size() - 1;
			case TRANSFER: this->oneShotPassesPack.transfer.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotPassesPack.transfer.size() - 1;
			default: this->oneShotPassesPack.compute.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->oneShotPassesPack.compute.size() - 1;
		}
	}
	else 
	{
		switch (queueFamilyEnum_)
		{
			case GRAPHICS: this->framePassesPack.graphics.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->framePassesPack.graphics.size() - 1;
			case TRANSFER: this->framePassesPack.transfer.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->framePassesPack.transfer.size() - 1;
			default: this->framePassesPack.compute.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); return this->framePassesPack.compute.size() - 1;
		}
	}
}
}
