#include "SyncManager.h"

namespace Graphics
{
PoolId SyncManager::addSemaphore(const char* name_)
{
	return this->semaphores.add(name_);
}

PoolId SyncManager::addFence(const char* name_, VkFenceCreateFlags flags_)
{
	return this->fences.add(name_, flags_);
}

Semaphore& SyncManager::getSemaphore(PoolId id_)
{
	return this->semaphores.get(id_); 
}

Fence& SyncManager::getFence(PoolId id_)
{
	return this->fences.get(id_);
}

void SyncManager::destroy()
{
	this->fences.clear(); 
	this->semaphores.clear();
}
}
