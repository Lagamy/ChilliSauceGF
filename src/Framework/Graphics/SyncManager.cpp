#include "SyncManager.h"

namespace Graphics
{
PoolId SyncManager::addSemaphore()
{
	PoolId pId = this->semaphores.add();
	this->semaphores.getInternal(pId.id).create(); 
	return pId;
}

PoolId SyncManager::addFence(VkFenceCreateFlags flags_)
{
	PoolId pId = this->fences.add();
	this->fences.getInternal(pId.id).create(flags_); 
	return pId;
}

Semaphore& SyncManager::getSemaphore(PoolId id_)
{
	return this->semaphores[id_]; 
}

Fence& SyncManager::getFence(PoolId id_)
{
	return this->fences[id_];
}

void SyncManager::destroy()
{
	this->fences.clear(); 
	this->semaphores.clear();
}
}
