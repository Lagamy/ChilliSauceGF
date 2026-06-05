#pragma once 
#include "Pool.h"
#include "Semaphore.h"
#include "Fence.h"

struct SyncManager { 
	Pool<Fence> fences; 
	Pool<Semaphore> semaphores;
};
