#include "FrameResources.h"
#include "Globals.h"

void FrameResources::create()
{	
	this->frameCommandPools.create();
	this->oneShotCommandPools.create();
}



void FrameResources::destroy()
{
	this->oneShotCommandPools.destroy();
	this->frameCommandPools.destroy();
}

VkCommandBuffer& FrameResources::getCommandBuffer(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamily_, uint32_t id_)
{
	if(poolType_ == FRAME) 
	{
		this->frameCommandPools.commandPools[queueFamily_].get(); 
	}
	else 
	{
		this->oneShotCommandPools.commandPools[queueFamily_].get(); 
	}
}
