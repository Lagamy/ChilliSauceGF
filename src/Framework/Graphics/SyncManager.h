#pragma once 
#include "PoolNameless.h"
#include "Semaphore.h"
#include "Fence.h"

namespace Graphics
{
struct SyncManager { 
	PoolNameless<Fence> fences = ("Fences"); 
	PoolNameless<Semaphore> semaphores = ("Semaphores");

	PoolId addSemaphore(); 
	PoolId addFence(VkFenceCreateFlags flags_);
	Semaphore& getSemaphore(PoolId id_);  
	Fence& getFence(PoolId id_); 
	void destroy();
};
}
