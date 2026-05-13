#include "FrameResources.h"
#include "Engine.h"

void FrameResources::init()
{
	this->mainCmdPoolPack.init(VK_COMMAND_BUFFER_LEVEL_PRIMARY); 
	this->secondaryCmdPoolPacks.resize(Engine::threadManager.workersCount); 

	for (auto& secondaryCMDPool : this->secondaryCmdPoolPacks)
	{
		secondaryCMDPool.init(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	}
}
