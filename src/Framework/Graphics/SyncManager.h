#pragma once 
#include "Pool.h"
#include "Semaphore.h"
#include "Fence.h"

namespace Graphics
{
struct SyncManager { 
	Pool<Fence> fences = ("Fences"); 
	Pool<Semaphore> semaphores = ("Semaphores");

	PoolId addSemaphore(); 
	PoolId addFence(VkFenceCreateFlags flags_);
	Semaphore& getSemaphore(PoolId id_);  
	Fence& getFence(PoolId id_); 
	void destroy();
};
}
