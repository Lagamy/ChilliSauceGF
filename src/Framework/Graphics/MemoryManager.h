// Represents resources that are in GPU currently
#pragma once 
#include "DynamicAllocator.h"
#include "MemoryEntryId.h"
#include "MemoryEntryCreateParams.h"
#include "StaticAllocator.h"
#include "StagingAllocator.h"
#include "QueueTransferSync.h"
#include "PushConstantStorage.h"
#include "UpdateRequest.h"
#include "PendingCopy.h"
#include "UploadBatch.h"
#include "ReadbackRequest.h"
#include "EntryCopyRequest.h"
#include "Utilities.h"
#include "Operation.h"
#include "GPUDataContainer.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void uploadQueuedCopiesCMDs(VkCommandBuffer& cmdBuffer_);

inline const uint32_t DefaultUploadSlotId = 0;

struct MemoryManager 
{
	StaticAllocator staticAllocator;
	DynamicAllocator dynamicAllocator;

	StagingAllocator stagingAllocator; // Staging ranges for initial uploads and updates. 
	QueueTransferSync queueTransferSync; // sharing mode of GPU_LOCAL buffers, set first in setup
	PushConstantStorage pushConstantStorage; // the push constant budget of the device, pipeline layouts place their push constant ranges in it

	// Only for GPU Local ones. 
	Pool<UpdateRequest> entriesPendingForUpdate;
	Pool<MemoryEntryId> entriesPendingForRemoval;
	Pool<ReadbackRequest> readbackRequests; // Alive from requestReadback until releaseReadback.
	Pool<EntryCopyRequest> entryCopyRequests; // Alive from copyEntry until its copies are queued.

	std::vector<Operation> pendingOperationsInOrder;
	std::vector<MemoryEntryId> copiedEntryIds; // Entries with a shadow (CPU_SHARED or GPU_LOCAL), the ones syncCopiesToFrame walks.
	
	/*
		GPU Local uploads are batched. Entries only queue a copy, and a ONESHOT TRANSFER pass records every copy queued in a slot into its command buffer.
		There is one slot, DefaultUploadSlotId, and queueCopy appends to it.
		Batch ids only grow. An entry is pending until isBatchCompleted(uploadBatchId).
	*/
	std::vector<UploadBatch> uploadBatches;
	uint64_t nextUploadBatchId = 1;

	void resolvePendingOperations(); // Checks every cycle -> isUploadPending for each entry. If true -> operation happens, false -> skip

	MemoryEntryId addEntry(const MemoryEntryCreateParams& rParams_); // params.data has to be set
	MemoryEntry& getEntry(MemoryEntryId memoryEntryId_);
	Buffer& getBuffer(const MemoryEntry& rEntry_);
	const MemoryEntry& getCopy(const MemoryEntry& rEntry_, uint32_t copyId_); // copy 0 is the entry itself, copyId_ is taken modulo copyCount
	const MemoryEntry& getFrameCopy(const MemoryEntry& rEntry_); // the copy of currentFrame (the entry itself when it has one copy). Bind this one: getBuffer + inBufferFirstByte
	void syncCopiesToFrame(uint32_t frame_); // copies every changed shadow into the frame's copy, runs in Renderer::draw right after the frame's fence wait
	bool isFrameCopyReady(MemoryEntryId memoryEntryId_); // an entry with copies: the copy of currentFrame was filled. One copy: its upload completed. drawMeshIndexed checks this
	bool isUploadInGPU(MemoryEntryId memoryEntryId_);
	bool hasPendingWrite(MemoryEntryId memoryEntryId_); // an upload is queued or in flight, or an UPDATE waits to be queued
	void updateEntry(MemoryEntryId entryId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_);
	void removeDynamicEntry(MemoryEntryId entryId_); 
	void queueEntryCopy(const EntryCopyRequest& rRequest_, MemoryEntry& rSrcEntry_, MemoryEntry& rDstEntry_);
	void copyEntry(MemoryEntryId srcEntryId_, MemoryEntryId dstEntryId_, const std::vector<MemoryCopyRegion>& regions_); // GPU Local -> GPU Local on the GPU, deferred like a readback. Empty regions = the first min(src size, dst size) bytes. The destination can have writes pending, the copy goes after them
	void zeroEntryRanges(MemoryEntryId entryId_, const std::vector<MemoryCopyRegion>& rRanges_); // Sets the byte ranges (MemoryCopyRegion::dstOffset, size) of an entry to 0: in memory for CPU_SHARED and entries with a shadow, into the staging range of a reserved entry, otherwise on the GPU
	std::vector<char> takeShadow(MemoryEntryId entryId_); // moves the shadow out of an entry with copies (the entry is about to be removed and its owner keeps the bytes, so there is one CPU copy, not two). Throws when the entry has none
	MemoryEntryId allocateEntry(const MemoryEntryCreateParams& rParams_); // params.data has to be nullptr
	MemoryEntryId reserveEntry(const MemoryEntryCreateParams& rParams_); // params.data has to be nullptr
	void commitEntry(MemoryEntryId entryId_);
	void lockEntry(MemoryEntryId entryId_);
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
	void writeShared(MemoryEntry& rEntry_, uint64_t inEntryOffset_, const void* pSrc_, uint64_t size_);
	void readShared(const MemoryEntry& rEntry_, uint64_t inEntryOffset_, void* pDst_, uint64_t size_);
	char* getSharedPointer(const MemoryEntry& rEntry_, uint64_t inEntryOffset_);
	uint64_t queueCopy(const PendingCopy& rCopy_);
	void updateUploadBatches();
	void startUploadBatch(uint32_t slotId_);
	void completeUploadBatch(uint32_t slotId_);
	bool isBatchCompleted(uint64_t batchId_);
	void waitForBatch(uint64_t batchId_); // blocks until the batch's fence is signaled, only after the submit that carries it (Renderer::draw calls it after resolveSync_SubmitToGPU). Does nothing for a batch that is not in flight

	void createCopies(MemoryEntryId entryId_, const MemoryAccessParams& rAccess_);

	void setup();
	void destroy();
};
}
