#include "RenderFlow.h"

void RenderFlow::addCmdBufferBlueprint(QueueFamilyEnum queueFamilyEnum_, recordFunc commandsToRecord_)
{
	switch (queueFamilyEnum_)
	{
		case GRAPHICS: this->graphicsCmdBufferBlueprints.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		case TRANSFER: this->transferCmdBufferBlueprints.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		case COMPUTE: this->computeCmdBufferBlueprints.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
	}
}

