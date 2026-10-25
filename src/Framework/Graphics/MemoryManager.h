// Represents resources that are in GPU currently
#pragma once 
#include "DynamicAllocator.h"
#include "MemoryEntryId.h"
#include "StaticAllocator.h"
#include "StagingAllocator.h"
#include "UpdateRequest.h"
#include "PendingCopy.h"
#include "ReadbackRequest.h"
#include "PassId.h"
#include "Utilities.h"
#include "Operation.h"
#include "GPUDataContainer.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void uploadQueuedCopiesCMDs(VkCommandBuffer& cmdBuffer_);

struct MemoryManager 
{
	StaticAllocator staticAllocator;
	DynamicAllocator dynamicAllocator;

	StagingAllocator stagingAllocator; // Staging ranges for initial uploads and updates. 

	// Only for GPU Local ones. 
	Pool<UpdateRequest> entriesPendingForUpdate;
	Pool<MemoryEntryId> entriesPendingForRemoval;
	Pool<ReadbackRequest> readbackRequests; // Alive from requestReadback until releaseReadback.

	std::vector<Operation> pendingOperationsInOrder;
	
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

	MemoryEntryId addEntry(const char* name_, const void* data_, VkDeviceSize size_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	MemoryEntry& getEntry(MemoryEntryId memoryEntryId_);
	Buffer& getBuffer(const MemoryEntry& rEntry_);
	bool isUploadInGPU(MemoryEntryId memoryEntryId_);
	void updateEntry(MemoryEntryId entryId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
	void removeDynamicEntry(MemoryEntryId entryId_); 
	MemoryEntryId reserveEntry(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum bufferType_, VkDeviceSize size_);
	void commitEntry(MemoryEntryId entryId_);
	StagingDataContainer getStagingContainer(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	SharedDataContainer getSharedContainer(MemoryEntryId entryId_);
	ReadbackId requestReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_);
	bool isReadbackReady(ReadbackId readbackId_);
	ReadbackDataContainer getReadbackContainer(ReadbackId readbackId_);
	void releaseReadback(ReadbackId readbackId_);
	StagingHeap& getStagingHeap(const StagingData& rStagingData_);

	/* The only place that copies bytes in or out of host-visible memory. Offsets are entry byte offsets. */
	void writeStaging(const StagingData& rStagingData_, uint64_t inEntryOffset_, const void* pSrc_, uint64_t size_);
	void readStaging(const StagingData& rStagingData_, uint64_t inEntryOffset_, void* pDst_, uint64_t size_);
	void writeShared(const MemoryEntry& rEntry_, uint64_t inEntryOffset_, const void* pSrc_, uint64_t size_);
	void readShared(const MemoryEntry& rEntry_, uint64_t inEntryOffset_, void* pDst_, uint64_t size_);
	char* getSharedPointer(const MemoryEntry& rEntry_, uint64_t inEntryOffset_);
	uint64_t queueCopy(const PendingCopy& rCopy_);
	void updateUploadBatches();

	void setup();
	void destroy();
};
}
