#pragma once 
#include "Pool.h"
#include "Semaphore.h"
#include "Fence.h"

namespace Graphics
{
struct SyncManager { 
	Pool<Fence> fences; 
	Pool<Semaphore> semaphores;

	PoolId addSemaphore(const char* name_); 
	PoolId addFence(const char* name_, VkFenceCreateFlags flags_);
	Semaphore& getSemaphore(PoolId id_);  
	Fence& getFence(PoolId id_); 
	void destroy();
};
}
