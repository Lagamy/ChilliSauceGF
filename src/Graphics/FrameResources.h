#pragma once
#include "Pool.h"
#include "CmdPoolsPack.h"
#include "FrameCommandPool.h"
#include "OneShotCommandPool.h"
#include "Semaphore.h"
#include "Utilities.h"

namespace Graphics
{
struct FrameResources {
	uint32_t id; 
	CmdPoolsPack<FrameCommandPool> frameCmdPools; 
	Semaphore imageAcquireSemaphore; 
	Fence frameAvailableFence; 

	void setup(uint32_t id_);
	void destroy();
};
}
