#include "MemoryManager.h"
#include "DynamicAllocator.h"
#include "Fence.h"
#include "Api.h"
#include "GPULocalPage.h"
#include "Globals.h"
#include "MemoryBlock.h"
#include "MemoryEntry.h"
#include "MemoryEntryId.h"
#include "Operation.h"
#include "Utilities.h"
#include <limits>
#include <stdexcept>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void MemoryManager::setup()
{
	this->staticAllocator.create(); 
	this->dynamicAllocator.init();
}

void MemoryManager::resolvePendingOperations()
{
	this->staticAllocator.checkUploadsStatus(); 
	for(uint32_t i = 0; i < this->pendingOperationsInOrder.size(); i++)
	{
		Operation& rPendingOperation = this->pendingOperationsInOrder[i]; 
		if(rPendingOperation.type == UPDATE)
		{
			UpdateRequest& rUpdateInfo = this->entriesPendingForUpdate[rPendingOperation.id]; 
			MemoryEntry& rMemoryEntry = this->getEntry(rUpdateInfo.memoryEntryId); 
			if(!rMemoryEntry.isPendingUpload()) // Then we can update it. 
			{
				if(rMemoryEntry.stagingData.heapId != UninitializedPoolId)
				{
					this->freeUpdateStagingHeaps.emplace_back(rMemoryEntry.stagingData.heapId);
					rMemoryEntry.stagingData.heapId = UninitializedPoolId;  
				}

				this->setStagingDataForUpdate(rMemoryEntry, rUpdateInfo);
				
				rMemoryEntry.upload(rUpdateInfo.data, rUpdateInfo.inSrcOffset, rUpdateInfo.inEntryOffset, rUpdateInfo.size); 
				this->entriesPendingForUpdate.remove(rPendingOperation.id); 
				this->pendingOperationsInOrder.erase(this->pendingOperationsInOrder.begin() + i); 
				i--;
			}
		}
		else 
		{
			MemoryEntry& rMemoryEntry = this->getEntry(this->entriesPendingForRemoval[rPendingOperation.id]); 
			if(!rMemoryEntry.isPendingUpload()) // Then we can remove it. 
			{
				this->dynamicAllocator.removeGPULocalEntry(this->entriesPendingForRemoval[rPendingOperation.id].id); 
				this->entriesPendingForRemoval.remove(rPendingOperation.id); 
				this->pendingOperationsInOrder.erase(this->pendingOperationsInOrder.begin() + i); 
				i--;
			}
		} 
	}
}


MemoryEntryId MemoryManager::addEntry(const char* name_, const void* data_, VkDeviceSize size_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_)
{
	PoolId id; 
	if(allocatorType_ == STATIC)
	{
		id = this->staticAllocator.addEntry(name_, data_, size_, memoryVisability_, uploadType_); 		
		return { STATIC, id};
	}
	else 
	{
		this->dynamicAllocator.addAndUploadEntry(name_, data_, size_, memoryVisability_, uploadType_);
		return { DYNAMIC, id}; 
	}
}


MemoryEntry& MemoryManager::getEntry(MemoryEntryId entryId_)
{
	if(entryId_.allocatorType == DYNAMIC)
	{
		return this->dynamicAllocator.memoryEntries[entryId_.id]; 
	}
		
	return this->staticAllocator.memoryEntries[entryId_.id.id];  
}

void MemoryManager::updateEntry(MemoryEntryId entryId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
{

	// Create Staging buffers or use existing one and remove it from free list, add id of them to Update var 
	MemoryEntry& rEntry = this->getEntry(entryId_); 

	if(rEntry.isForDynamic)
	{
		PoolId opId = this->entriesPendingForUpdate.add(entryId_, data_, inSrcOffset_, inEntryOffset_, size_);
		this->pendingOperationsInOrder.emplace_back(UPDATE, opId); 
	}
	else
	{
		rEntry.upload(data_, inSrcOffset_, inEntryOffset_, size_); 
	}
} 


void MemoryManager::setStagingDataForUpdate(MemoryEntry& rEntry_, UpdateRequest& rUpdateRequest_)
{
	if(this->freeUpdateStagingHeaps.size() < Globals::maxDormantUpdateStagingHeaps)
	{
		for(PoolId& id : this->freeUpdateStagingHeaps)
		{
			this->uploadStagingHeaps.remove(id); 
		}
		this->freeUpdateStagingHeaps.clear(); 
	}

    if(this->freeUpdateStagingHeaps.empty())
    {
        PoolId id = this->uploadStagingHeaps.add(rUpdateRequest_.size);
		rEntry_.stagingData = { id, rUpdateRequest_.inEntryOffset, rUpdateRequest_.size }; 
		return;
	}
	
	uint32_t attemptsToFindSuitable = 0; 
	for(uint32_t i = 1; i < this->freeUpdateStagingHeaps.size(); i++)
	{
		if(attemptsToFindSuitable >= Globals::maxAttemptsToUseExistingStagingHeaps)
			break; 

		StagingHeap& rHeap = this->uploadStagingHeaps[this->freeUpdateStagingHeaps[i]]; 
		if(rHeap.size <= rUpdateRequest_.size)
		{
			rEntry_.stagingData = {this->freeUpdateStagingHeaps[i], rUpdateRequest_.inEntryOffset, rUpdateRequest_.size}; 				
			this->freeUpdateStagingHeaps.erase(this->freeUpdateStagingHeaps.begin() + i); 
			return; 
		}

	}

	PoolId id = this->uploadStagingHeaps.add(rUpdateRequest_.size);
	rEntry_.stagingData = { id, rUpdateRequest_.inEntryOffset, rUpdateRequest_.size}; 
	return;
}

void MemoryManager::removeDynamicEntry(MemoryEntryId entryId_)
{
	#ifdef ENGINE_DEBUG
		if(entryId_.allocatorType == STATIC)
		{
			throw std::runtime_error("Memory Manager: Cant remove statically allocated memory entry."); 	
		}
	#endif 

	

	MemoryEntry& rMemoryEntry = this->dynamicAllocator.memoryEntries[entryId_.id]; 

	if(rMemoryEntry.isForDynamic)
	{
		PoolId id = this->entriesPendingForRemoval.add(entryId_);
		this->pendingOperationsInOrder.emplace_back(REMOVE, id); 
		return; 
	}

	this->dynamicAllocator.removeCPUSharedEntry(entryId_.id);
	// Add staging heap connected to this update to the free Staging heap list.  
}

void MemoryManager::destroy() 
{
	this->staticAllocator.deallocate();
	this->dynamicAllocator.deallocate(); 
}
}
