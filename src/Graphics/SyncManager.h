#pragma once 
#include "Pool.h"
#include "Semaphore.h"
#include "Fence.h"

namespace Graphics
{
struct SyncManager { 
	Pool<Fence> fences; 
	Pool<Semaphore> semaphores;

	PoolId addUserSemaphore(const char* name_); 
	PoolId addUserFence(const char* name_, VkFenceCreateFlags flags_);
	Semaphore& getUserSemaphore(PoolId id_);  
	Fence& getUserFence(PoolId id_); 
	void destroy();
};
}
