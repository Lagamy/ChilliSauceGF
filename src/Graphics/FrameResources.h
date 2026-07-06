#pragma once
#include "Pool.h"
#include "CmdPoolsPack.h"
#include "FrameCommandPool.h"
#include "OneShotCommandPool.h"
#include "CommandBufferBlueprint.h"
#include "Semaphore.h"
#include "Utilities.h"

namespace Graphics
{
struct FrameResources {
	uint32_t id; 
	CmdPoolsPack<FrameCommandPool> frameCmdPools; 
	PoolId imageAvailableSemaphoreId; 
	PoolId frameAvailableFenceId; 

	void setup(uint32_t id_);
};
}
