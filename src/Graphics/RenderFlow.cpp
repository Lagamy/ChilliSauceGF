#include "RenderFlow.h"

void RenderFlow::initCmdBufferBlueprint(CmdBufferTypeEnum cmdBufferType_, Cmd commandsToRecord_)
{
	switch (cmdBufferType_)
	{
		case GRAPHICS: this->graphicsCmdBufferBlueprint.init(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		case TRANSFER: this->transferCmdBufferBlueprint.init(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		case COMPUTE: this->computeCmdBufferBlueprint.init(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
	}
}

