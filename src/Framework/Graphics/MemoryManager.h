// Represents resources that are in GPU currently
#pragma once 
#include "DynamicAllocator.h"
#include "MemoryEntryId.h"
#include "StaticAllocator.h"
#include "UpdateRequest.h"
#include "PendingCopy.h"
#include "PassId.h"
#include "Utilities.h"
#include "Operation.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void uploadQueuedCopiesCMDs(VkCommandBuffer& cmdBuffer_);

struct MemoryManager 
{
	StaticAllocator staticAllocator;
	DynamicAllocator dynamicAllocator;

	Pool<StagingHeap> uploadStagingHeaps; // Inside of each heap -> there would be several updates that could fit. If
	std::vector<PoolId> freeUpdateStagingHeaps; // If more than maxDormantUpdateStagingHeaps -> they will be freed. 

	// Only for GPU Local ones. 
	Pool<UpdateRequest> entriesPendingForUpdate;
	Pool<MemoryEntryId> entriesPendingForRemoval;
	Pool<MemoryEntry> dynamicEntriesPendingForInitUpload; 

	std::vector<Operation> pendingOperationsInOrder;
	std::vector<Operation> inProgressOperations; // For dynamic page related staging heap check/decrement of pendingUploadsCount / adding it to pages free staging heaps list if pendingUploadsCount == 0. 
	
	/*
		GPU Local uploads are batched. Entries only queue a copy, and one ONESHOT TRANSFER pass records every queued copy into a single command buffer.
		Batch ids only grow. An entry is pending while its uploadBatchId is bigger than completedUploadBatchId.
	*/
	PoolId uploadFenceId;
	PassId uploadPassId;
	std::vector<PendingCopy> queuedCopies;
	std::vector<PendingCopy> recordedCopies;
	uint64_t queuedUploadBatchId = 1;
	uint64_t inFlightUploadBatchId = 0;
	uint64_t completedUploadBatchId = 0;
	bool uploadBatchInFlight = false;

	void resolvePendingOperations(); // Checks every cycle -> isUploadPending for each entry. If true -> operation happens, false -> skip
	void resolveInProgressOperations(); 

	MemoryEntryId addEntry(const char* name_, const void* data_, VkDeviceSize size_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	MemoryEntry& getEntry(MemoryEntryId memoryEntryId_);
	Buffer& getBuffer(const MemoryEntry& rEntry_);
	bool isUploadInGPU(MemoryEntryId memoryEntryId_);
	void updateEntry(MemoryEntryId entryId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
	void removeDynamicEntry(MemoryEntryId entryId_); 
	void setStagingDataForUpdate(MemoryEntry& rEntry_, UpdateRequest& rUpdateRequest_); // Note: Allocation churn is possible, if updates are all different sizes
	uint64_t queueCopy(const PendingCopy& rCopy_);
	void updateUploadBatches();

	void setup();
	void destroy();
};
}
