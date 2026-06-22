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
	CmdPoolsPack<FrameCommandPool> frameCmdPools; 
	PoolId frameFinishedSemaphoreId;
	PoolId frameAvailableSemaphoreId; 
	PoolId frameAvailableFenceId; 

	void setup();
};
}
