#pragma once
#include "CommandPoolsPack.h"
#include "OneShotCommandPool.h"
#include "FrameCommandPool.h"
#include "CommandBufferBlueprint.h"
#include "Semaphore.h"
#include "Utilities.h"

struct FrameResources {
	CommandPoolsPack<OneShotCommandPool> oneShotCommandPools; 
	CommandPoolsPack<FrameCommandPool> frameCommandPools; 

	void create();
	void destroy(); 
	VkCommandBuffer& getCommandBuffer(CommandPoolTypeEnum poolType_, QueueFamilyEnum queueFamily_, uint32_t id_);
};
