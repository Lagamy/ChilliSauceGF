#pragma once
#include "Pool.h"
#include "CommandPoolsPack.h"
#include "OneShotCommandPool.h"
#include "FrameCommandPool.h"
#include "CommandBufferBlueprint.h"
#include "Semaphore.h"
#include "Utilities.h"

struct FrameResources {
	CommandPoolsPack<FrameCommandPool> frameCmdPools; 
	PoolId frameFinishedFenceId; 
};
