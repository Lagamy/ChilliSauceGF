// Represents resources that are in GPU currently
#pragma once 
#include "DynamicAllocator.h"
#include "MemoryEntryId.h"
#include "StaticAllocator.h"
#include "UpdateRequest.h"
#include "Utilities.h"
#include "Operation.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{

struct MemoryManager 
{
	StaticAllocator staticAllocator;
	DynamicAllocator dynamicAllocator;

	Pool<StagingHeap> uploadStagingHeaps; // Inside of each heap -> there would be several updates that could fit. If
	std::vector<PoolId> freeUpdateStagingHeaps; // If more than maxDormantUpdateStagingHeaps -> they will be freed. 

	// Only for GPU Local ones. 
	Pool<UpdateRequest> entriesPendingForUpdate;
	Pool<MemoryEntryId> entriesPendingForRemoval;
	std::vector<Operation> pendingOperationsInOrder;
	
	// Todo: add multithreading and make it a per thread var 
    PoolId currentEntryToUpload;
	
	void resolvePendingOperations(); // Checks every cycle -> isUploadPending for each entry. If true -> operation happens, false -> skip
	
	MemoryEntryId addEntry(const char* name_, const void* data_, VkDeviceSize size_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	MemoryEntry& getEntry(MemoryEntryId memoryEntryId_);
	void updateEntry(MemoryEntryId entryId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
	void removeDynamicEntry(MemoryEntryId entryId_); 
	void setStagingDataForUpdate(MemoryEntry& rEntry_, UpdateRequest& rUpdateRequest_); // Note: Allocation churn is possible, if updates are all different sizes 

	void setup();
	void destroy();
};
}
