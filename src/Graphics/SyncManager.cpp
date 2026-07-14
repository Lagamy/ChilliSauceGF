#include "SyncManager.h"

namespace Graphics
{
PoolId SyncManager::addSemaphore(const char* name_)
{
	PoolId pId = this->semaphores.add(name_);
	this->semaphores.getInternal(pId.id).create(); 
	return pId;
}

PoolId SyncManager::addFence(const char* name_, VkFenceCreateFlags flags_)
{
	PoolId pId = this->fences.add(name_);
	this->fences.getInternal(pId.id).create(flags_); 
	return pId;
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
