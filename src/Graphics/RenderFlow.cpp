#include "RenderFlow.h"

void RenderFlow::initPrimaryCmdBufferBlueprint(CmdBufferTypeEnum cmdBufferType_, Cmd commandsToRecord_)
{
	switch (cmdBufferType_)
	{
		case GRAPHICS: this->primaryGraphicsCmdBufferBlueprint.init(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		case TRANSFER: this->primaryTransferCmdBufferBlueprint.init(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
		case COMPUTE: this->primaryComputeCmdBufferBlueprint.init(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); break;
	}
}

void  RenderFlow::addSecondaryCmdBufferBlueprint(CmdBufferTypeEnum cmdBufferType_, Cmd commandsToRecord_)
{
	switch (cmdBufferType_)
	{
		case GRAPHICS: this->secondaryGraphicsCmdBufferBlueprints.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT); break;
		case TRANSFER: this->secondaryTransferCmdBufferBlueprints.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT); break;
		case COMPUTE: this->secondaryComputeCmdBufferBlueprints.emplace_back(commandsToRecord_, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT); break;
	}

}
