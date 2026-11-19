#include "MemoryManager.h"
#include <limits>
#include "DynamicAllocator.h"
#include "Fence.h"
#include "Api.h"
#include "GPULocalPage.h"
#include "Globals.h"
#include "MemoryBlock.h"
#include "MemoryEntry.h"
#include "MemoryEntryId.h"
#include "CPUSharedPage.h"
#include "Operation.h"
#include "Utilities.h"
#include <algorithm>
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
/* Every stage that reads an entry buffer in a draw or dispatch: vertex input (vertex and index buffers), all graphics shader stages and compute (uniform and storage buffers). */
static const VkPipelineStageFlags DrawReadStages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

void uploadQueuedCopiesCMDs(VkCommandBuffer& cmdBuffer_)
{
	MemoryManager& rMemoryManager = getMemoryManager();

	/* Every slot has its own command buffer but shares this function, so the slot is the one that owns the handle being recorded. */
	UploadBatch* pBatch = nullptr;
	for(UploadBatch& rBatch : rMemoryManager.uploadBatches)
	{
		if(getCommandBuffer(TRANSFER, ONESHOT, rBatch.cmdId) == cmdBuffer_)
		{
			pBatch = &rBatch;
			break;
		}
	}
	#ifdef ENGINE_DEBUG
		if(pBatch == nullptr)
		{
			throw std::runtime_error("Memory Manager: Upload command buffer doesn't belong to any upload batch.");
		}
	#endif

	/*
		The first synchronization scope of a pipeline barrier includes every command earlier in submission order on the queue, also from earlier vkQueueSubmit calls (spec: Synchronization and Cache Control -> Pipeline Barriers).
		So this TRANSFER -> TRANSFER dependency orders the batch after earlier batches, e.g. an update write must not overtake a readback copy that reads the same entry buffer.
		Only one batch is in flight at a time (the next starts after the previous fence was observed signaled), so the CPU already guarantees this order. The barrier is a second guard.
		The graphics and compute stages in the source scope order the batch after the draws of earlier frames, which may still read the entry buffers: a write after a read only needs this execution dependency, no access masks.
		It works because TRANSFER and GRAPHICS use one queue. With separate families a barrier only orders its own queue, and a transfer-only queue family doesn't support graphics or compute stages in a barrier (spec: vkCmdPipelineBarrier, stage support per queue family),
		so the source is TRANSFER only and the done semaphores (QueueTransferSync) order the batch after the other queues' work.
	*/
	VkPipelineStageFlags batchStartStages = VK_PIPELINE_STAGE_TRANSFER_BIT | (rMemoryManager.queueTransferSync.separateFamilies ? 0 : DrawReadStages);
	VkMemoryBarrier batchStartBarrier = {};
	batchStartBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	batchStartBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	batchStartBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(cmdBuffer_, batchStartStages, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &batchStartBarrier, 0, nullptr, 0, nullptr);

	/*
		Copies of one batch run in recorded order with no implicit ordering between them. A readback copy reads an entry buffer that an update copy in the same batch may write
		(a write after a read), so a TRANSFER -> TRANSFER memory dependency is recorded wherever the kind of copy switches.
	*/
	bool hasReadback = false;
	bool hasUpdate = false;
	for(uint32_t i = 0; i < pBatch->recordedCopies.size(); i++)
	{
		const PendingCopy& rCopy = pBatch->recordedCopies[i];
		if(i > 0 && rCopy.isReadback != pBatch->recordedCopies[i - 1].isReadback)
		{
			VkMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cmdBuffer_, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		}
		hasReadback = hasReadback || rCopy.isReadback;
		hasUpdate = hasUpdate || !rCopy.isReadback;

		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = rCopy.srcOffset;
		bufferCopyRegion.dstOffset = rCopy.dstOffset;
		bufferCopyRegion.size = rCopy.size;
		vkCmdCopyBuffer(cmdBuffer_, rCopy.srcBuffer, rCopy.dstBuffer, 1, &bufferCopyRegion);
	}

	/* Makes the readback copies' writes available to the host. The CPU reads them after the batch fence is observed signaled (see isReadbackReady). */
	if(hasReadback)
	{
		VkMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
		vkCmdPipelineBarrier(cmdBuffer_, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
	}

	/*
		Makes the update copies' writes available and visible to the draws and dispatches that read the entry buffers (vertex, index, uniform and storage reads). Without it nothing orders a draw after the copy:
		the second synchronization scope of a pipeline barrier includes every command later in submission order on the queue, and the transfer pass is submitted before this frame's graphics passes.
		Skipped when the batch only holds readbacks, which write staging memory.
		Also skipped with separate queue families: the barrier would not reach the other queues, and the batch's signal of the transfer done semaphore makes the writes available and visible to the commands that wait for it (spec: Synchronization and Cache Control -> Semaphores).
	*/
	if(hasUpdate && !rMemoryManager.queueTransferSync.separateFamilies)
	{
		VkMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(cmdBuffer_, VK_PIPELINE_STAGE_TRANSFER_BIT, DrawReadStages, 0, 1, &barrier, 0, nullptr, 0, nullptr);
	}
}

void MemoryManager::setup()
{
	/*
		The queue families are resolved first: GPU_LOCAL buffers created by the allocators take their sharing mode from it.
		uploadQueuedCopiesCMDs orders the batch against draws with barriers when TRANSFER and GRAPHICS share a queue, and with semaphores when they do not.
	*/
	this->queueTransferSync.setup();

	this->dynamicAllocator.init();
	this->stagingAllocator.init();

	/* One list for every slot: the command buffer being recorded tells uploadQueuedCopiesCMDs which slot it is. The list begins and ends the command buffer. */
	PoolId uploadCommandListId = addCommandList("Memory Manager Upload Commands");
	getCommandList(uploadCommandListId).addUploadQueuedCopies();
	this->uploadBatches.resize(1);
	for(uint32_t i = 0; i < this->uploadBatches.size(); i++)
	{
		UploadBatch& rBatch = this->uploadBatches[i];
		rBatch.fenceId = addFence(true);
		rBatch.passId = addPass(std::format("Memory Manager Upload Batch {}", i).c_str(), ONESHOT, TRANSFER, rBatch.fenceId);
		uint32_t taskId = addTaskToPass(rBatch.passId, "Queued Copies Upload", uploadCommandListId);
		rBatch.cmdId = getPass(rBatch.passId).tasks[taskId].cmdIds[0];
	}
}

uint64_t MemoryManager::queueCopy(const PendingCopy& rCopy_)
{
	UploadBatch& rBatch = this->uploadBatches[DefaultUploadSlotId];
	if(rBatch.queuedCopies.empty())
	{
		rBatch.queuedBatchId = this->nextUploadBatchId;
		this->nextUploadBatchId++;
	}
	rBatch.queuedCopies.emplace_back(rCopy_);
	return rBatch.queuedBatchId;
}

/*
	Batch ids only grow and a slot carries an id from its first queued copy until its fence was observed signaled, so an id that no slot carries anymore is complete.
	Completion is per batch. Batches on one queue can finish out of order, so one batch's fence says nothing about the others.
	0 is the id of an entry with nothing uploaded, OpenBatchId is a copy that is not queued yet.
*/
bool MemoryManager::isBatchCompleted(uint64_t batchId_)
{
	if(batchId_ == OpenBatchId)
	{
		return false;
	}
	if(batchId_ == 0)
	{
		return true;
	}

	for(const UploadBatch& rBatch : this->uploadBatches)
	{
		if(rBatch.queuedBatchId == batchId_ || (rBatch.inFlight && rBatch.recordedBatchId == batchId_))
		{
			return false;
		}
	}
	return true;
}

/*
	The fence of a started batch is reset in startUploadBatch and only signaled once the pass was submitted and finished, so waiting for it before the submit would never return.
	The batch is not marked completed here: updateUploadBatches does that at the next frame start, like for every batch.
*/
void MemoryManager::waitForBatch(uint64_t batchId_)
{
	for(const UploadBatch& rBatch : this->uploadBatches)
	{
		if(rBatch.inFlight && rBatch.recordedBatchId == batchId_)
		{
			VkFence fence = getFence(rBatch.fenceId);
			vkWaitForFences(getMainDevice().logicalDevice, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		}
	}
}

void MemoryManager::startUploadBatch(uint32_t slotId_)
{
	UploadBatch& rBatch = this->uploadBatches[slotId_];
	rBatch.recordedCopies = std::move(rBatch.queuedCopies);
	rBatch.queuedCopies.clear();
	rBatch.recordedBatchId = rBatch.queuedBatchId;
	rBatch.queuedBatchId = 0;
	resetFence(rBatch.fenceId);
	enablePass(rBatch.passId);
	rBatch.inFlight = true;
}

/* Called once the slot's fence was observed signaled, so its copies stopped reading the staging ranges tagged with its id. */
void MemoryManager::completeUploadBatch(uint32_t slotId_)
{
	UploadBatch& rBatch = this->uploadBatches[slotId_];
	rBatch.inFlight = false;
	this->stagingAllocator.releaseBatch(rBatch.recordedBatchId);
}

void MemoryManager::updateUploadBatches()
{
	/*
		A slot's fence is only reset once the CPU saw it signaled, since vkResetFences requires the fence to have no pending submission.
		That also means the slot's command buffer finished executing, so it is safe to re-record it and to reuse its staging buffers.
		There is no semaphore on these passes: a binary semaphore signaled every batch and never waited on could not be signaled again.
	*/
	for(uint32_t i = 0; i < this->uploadBatches.size(); i++)
	{
		if(this->uploadBatches[i].inFlight && wasFenceSignaled(this->uploadBatches[i].fenceId))
		{
			this->completeUploadBatch(i);
		}
	}

	UploadBatch& rDefaultBatch = this->uploadBatches[DefaultUploadSlotId];
	if(!rDefaultBatch.inFlight && !rDefaultBatch.queuedCopies.empty())
	{
		this->startUploadBatch(DefaultUploadSlotId);
	}
}

void MemoryManager::resolvePendingOperations()
{
	this->updateUploadBatches();

	for(uint32_t i = 0; i < this->pendingOperationsInOrder.size(); i++)
	{
		Operation& rPendingOperation = this->pendingOperationsInOrder[i]; 
		if(rPendingOperation.type == UPDATE)
		{
			UpdateRequest& rUpdateInfo = this->entriesPendingForUpdate[rPendingOperation.id]; 
			MemoryEntry& rMemoryEntry = this->getEntry(rUpdateInfo.memoryEntryId); 
			/* A readback requested before this update must see the bytes from before it. The update waits until that readback copy completed, so the order is decided here on the CPU and not only by barriers. */
			if(!rMemoryEntry.isPendingUpload() && !rMemoryEntry.isPendingReadback()) // Then we can update it.
			{
				rMemoryEntry.queueStagedCopy(rUpdateInfo.stagingData, rUpdateInfo.stagingData.inEntryOffset, rUpdateInfo.stagingData.size);
				this->entriesPendingForUpdate.remove(rPendingOperation.id); 
				this->pendingOperationsInOrder.erase(this->pendingOperationsInOrder.begin() + i); 
				i--;
			}
		}
		else if(rPendingOperation.type == READBACK_COPY)
		{
			ReadbackRequest& rRequest = this->readbackRequests[rPendingOperation.id];
			MemoryEntry& rMemoryEntry = this->getEntry(rRequest.memoryEntryId);
			if(!rMemoryEntry.isPendingUpload()) // Then its bytes are final and we can read them back.
			{
				rRequest.batchId = rMemoryEntry.queueReadbackCopy(rRequest.stagingData, rRequest.inEntryOffset, rRequest.size);
				this->pendingOperationsInOrder.erase(this->pendingOperationsInOrder.begin() + i);
				i--;
			}
		}
		else 
		{
			MemoryEntry& rMemoryEntry = this->getEntry(this->entriesPendingForRemoval[rPendingOperation.id]); 
			/* A queued or in-flight readback copy still reads the entry's buffer, and destroying a buffer that pending commands use is invalid. So removal also waits for the readback batch. */
			if(!rMemoryEntry.isPendingUpload() && !rMemoryEntry.isPendingReadback()) // Then we can remove it.
			{
				this->dynamicAllocator.removeGPULocalEntry(this->entriesPendingForRemoval[rPendingOperation.id].id); 
				this->entriesPendingForRemoval.remove(rPendingOperation.id);
				this->pendingOperationsInOrder.erase(this->pendingOperationsInOrder.begin() + i);
				i--;
			}
		}
	}

	/* Updates resolved above queued new copies. Starting the batch here submits them this frame, since passes are compiled after this function. */
	this->updateUploadBatches();
}


MemoryEntryId MemoryManager::addEntry(const MemoryEntryCreateParams& rParams_)
{
	rParams_.validate();
	if(rParams_.data == nullptr)
	{
		throw std::runtime_error(std::format("Memory Manager: Can't add entry {} without data, use allocateEntry or reserveEntry.", rParams_.name));
	}

	MemoryEntryId entryId;
	if(rParams_.allocatorType == STATIC_ALLOCATOR)
	{
		entryId = { STATIC_ALLOCATOR, this->staticAllocator.addEntry(rParams_.name, rParams_.data, rParams_.size, rParams_.memoryVisability, rParams_.bufferType) };
	}
	else
	{
		entryId = { DYNAMIC_ALLOCATOR, this->dynamicAllocator.addAndUploadEntry(rParams_.name, rParams_.data, rParams_.size, rParams_.memoryVisability, rParams_.bufferType) };
	}
	this->getEntry(entryId).readOnly = rParams_.access.isReadOnly();
	this->createCopies(entryId, rParams_.access);

	/*
		Copy 0 gets the initial data from the upload (written before the copies existed for a dynamic entry, by allocateAndUpload for a static one). With copies the shadow is the source of truth, so the data goes there too,
		and copy 0 already counts as filled with that version. The other copies are filled by syncCopiesToFrame.
	*/
	if(this->getEntry(entryId).hasShadow)
	{
		MemoryEntry& rCopiedEntry = this->getEntry(entryId);
		this->writeShared(rCopiedEntry, 0, rParams_.data, rParams_.size);
		rCopiedEntry.copyVersions[0] = rCopiedEntry.version;
	}
	return entryId;
}

/*
	Gives a READ_AND_WRITE entry its copies and shadow (see CopyCountModeEnum). Copy 0 is the entry itself, every other copy is a hidden entry of the same allocator and visability that only owns memory (no data, nothing to upload).
	An entry with a single buffer has no shadow and its writes stay direct (MemoryAccessParams::hasShadow): ONE warns, and any other mode with a count of 1 makes a CPU_SHARED write wait for every frame first (waitsBeforeWrite).
	A GPU_LOCAL single buffer needs nothing more, its copies are ordered by the transfer barriers.
	Static copies have to exist before StaticAllocator::allocateAndUpload like any static entry. Every entry is fetched again after an add, since adding grows the allocator's storage.
*/
void MemoryManager::createCopies(MemoryEntryId entryId_, const MemoryAccessParams& rAccess_)
{
	uint32_t copyCount = rAccess_.resolveCopyCount();
	if(!rAccess_.hasShadow())
	{
		MemoryEntry& rSingleEntry = this->getEntry(entryId_);
		if(rAccess_.copyCountMode != CopyCountModeEnum::ONE && rSingleEntry.memoryVisability == CPU_SHARED)
		{
			rSingleEntry.waitsBeforeWrite = true;
		}
		if(rAccess_.copyCountMode != CopyCountModeEnum::ONE || rAccess_.isReadOnly())
		{
			return;
		}
		if(rSingleEntry.memoryVisability == GPU_LOCAL)
		{
			std::cerr << std::format("Memory Manager warning: Entry {} is READ_AND_WRITE and GPU_LOCAL with CopyCountModeEnum::ONE. Its writes can't tear, but each copy waits for the draws of earlier frames and later draws wait for it, so the GPU serializes the frames around every write.\n", rSingleEntry.name);
		}
		if(rSingleEntry.memoryVisability == CPU_SHARED)
		{
			std::cerr << std::format("Memory Manager warning: Entry {} is READ_AND_WRITE and CPU_SHARED with CopyCountModeEnum::ONE. Its writes can land while a frame in flight still reads it, which can cause graphical glitches.\n", rSingleEntry.name);
		}
		return;
	}

	const MemoryEntry& rEntry = this->getEntry(entryId_);

	std::string name = rEntry.name;
	VkDeviceSize size = rEntry.size;
	BufferTypeEnum bufferType = rEntry.bufferType;
	MemoryVisabilityEnum memoryVisability = rEntry.memoryVisability;
	for(uint32_t copy = 1; copy < copyCount; copy++)
	{
		std::string copyName = std::format("{} copy {}", name, copy);
		MemoryEntryId copyId;
		if(entryId_.allocatorType == STATIC_ALLOCATOR)
		{
			copyId = { STATIC_ALLOCATOR, this->staticAllocator.addEntry(copyName.c_str(), nullptr, size, memoryVisability, bufferType) };
		}
		else
		{
			copyId = { DYNAMIC_ALLOCATOR, this->dynamicAllocator.allocateEntry(copyName.c_str(), size, memoryVisability, bufferType) };
		}
		this->getEntry(entryId_).copyEntryIds.emplace_back(copyId);
	}

	MemoryEntry& rCopiedEntry = this->getEntry(entryId_);
	rCopiedEntry.copyCount = copyCount;
	rCopiedEntry.hasShadow = true;
	rCopiedEntry.shadow.assign(size, 0);
	rCopiedEntry.copyVersions.assign(copyCount, 0);
	rCopiedEntry.copySettled.assign(copyCount, 0);
	rCopiedEntry.waitsForSharingFrames = rAccess_.copyCountMode == CopyCountModeEnum::CUSTOM_COPY_COUNT_WITH_WAIT && copyCount < Globals::renderer.framesAtFlightCount && memoryVisability == CPU_SHARED;
	this->copiedEntryIds.emplace_back(entryId_);
}

/*
	Runs right after passesGraph.waitForFrame(frame_): the frame's previous submission finished (write after read), so its copy can be overwritten.
	The copy a frame uses is frame_ % copyCount. It is refreshed only when the shadow changed since that copy was last filled, always the whole entry.
	CPU_SHARED memory is coherent, so no flush is needed, and a host write before vkQueueSubmit is visible to that submission.
	A GPU_LOCAL copy is refreshed through a whole-entry staging range and an UPDATE operation, like any GPU_LOCAL write. Nothing in it belongs to a frame: the copy is queued once the copy entry has no upload in flight, and the batch barriers order it after earlier draws
	(so it never overwrites bytes an earlier frame still reads) and before later ones. The version is recorded when the bytes were staged, so a later change stages again and the updates of one copy run in order.
	A reserved entry that was not committed yet has nothing to sync.
*/
void MemoryManager::syncCopiesToFrame(uint32_t frame_)
{
	for(const MemoryEntryId& rEntryId : this->copiedEntryIds)
	{
		MemoryEntry& rEntry = this->getEntry(rEntryId);
		uint32_t copyId = frame_ % rEntry.copyCount;
		if(rEntry.awaitingCommit || rEntry.copyVersions[copyId] == rEntry.version)
		{
			continue;
		}

		MemoryEntryId copyEntryId = copyId == 0 ? rEntryId : rEntry.copyEntryIds[copyId - 1];
		if(rEntry.memoryVisability == GPU_LOCAL)
		{
			StagingDataContainer container = this->getStagingContainer(copyEntryId, 0, rEntry.size);
			container.write(0, rEntry.shadow.data(), rEntry.size);
		}
		else
		{
			/* Frames share this copy (CUSTOM_COPY_COUNT_WITH_WAIT): every frame that reads it has to be finished before it is overwritten. frame_ itself was waited on by the caller, waitForFrame is cheap on a signaled fence. */
			if(rEntry.waitsForSharingFrames)
			{
				for(uint32_t frame = copyId; frame < Globals::renderer.framesAtFlightCount; frame += rEntry.copyCount)
				{
					Globals::renderer.passesGraph.waitForFrame(frame);
				}
			}
			std::memcpy(this->getSharedPointer(this->getEntry(copyEntryId), 0), rEntry.shadow.data(), rEntry.size);
		}
		rEntry.copyVersions[copyId] = rEntry.version;
	}
}

/* Copy 0 is the entry itself, 1..copyCount - 1 are its hidden entries. */
const MemoryEntry& MemoryManager::getCopy(const MemoryEntry& rEntry_, uint32_t copyId_)
{
	uint32_t copyId = copyId_ % rEntry_.copyCount;
	if(copyId == 0)
	{
		return rEntry_;
	}
	return this->getEntry(rEntry_.copyEntryIds[copyId - 1]);
}

/* The copy that currentFrame reads. Bind this one (getBuffer + inBufferFirstByte), the entry itself is only the first copy. */
const MemoryEntry& MemoryManager::getFrameCopy(const MemoryEntry& rEntry_)
{
	uint32_t copyId = Globals::renderer.currentFrame % rEntry_.copyCount;
	if(copyId == 0)
	{
		return rEntry_;
	}
	return this->getEntry(rEntry_.copyEntryIds[copyId - 1]);
}


/*
	An entry with one copy is ready once its upload completed. With copies, the copy of currentFrame is ready once it was filled at least once (syncCopiesToFrame set its version) and that first fill finished
	(a CPU_SHARED copy is written at once, a GPU_LOCAL one when its UPDATE batch completed). Later refreshes don't make it unready: until they complete the copy still holds the previous, consistent bytes. Once settled it stays settled.
*/
bool MemoryManager::isFrameCopyReady(MemoryEntryId entryId_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	if(!rEntry.hasShadow)
	{
		return this->isUploadInGPU(entryId_);
	}

	uint32_t copyId = Globals::renderer.currentFrame % rEntry.copyCount;
	if(!rEntry.copySettled[copyId])
	{
		MemoryEntryId copyEntryId = copyId == 0 ? entryId_ : rEntry.copyEntryIds[copyId - 1];
		if(rEntry.copyVersions[copyId] == 0 || this->hasPendingWrite(copyEntryId))
		{
			return false;
		}
		rEntry.copySettled[copyId] = 1;
	}
	return true;
}

bool MemoryManager::isUploadInGPU(MemoryEntryId entryId_)
{
	return !this->getEntry(entryId_).isPendingUpload();
}

/* An UPDATE stays in entriesPendingForUpdate until resolvePendingOperations queues its copy, after that the entry's upload batch covers it. */
bool MemoryManager::hasPendingWrite(MemoryEntryId entryId_)
{
	if(this->getEntry(entryId_).isPendingUpload())
	{
		return true;
	}

	for(uint32_t i = 0; i < this->entriesPendingForUpdate.allocatedSize(); i++)
	{
		if(!this->entriesPendingForUpdate.alive[i])
		{
			continue;
		}

		const MemoryEntryId& pendingId = this->entriesPendingForUpdate.getInternal(i).memoryEntryId;
		if(pendingId.allocatorType == entryId_.allocatorType && pendingId.id == entryId_.id)
		{
			return true;
		}
	}
	return false;
}

MemoryEntry& MemoryManager::getEntry(MemoryEntryId entryId_)
{
	if(entryId_.allocatorType == DYNAMIC_ALLOCATOR)
	{
		return this->dynamicAllocator.memoryEntries[entryId_.id]; 
	}
		
	return this->staticAllocator.memoryEntries[entryId_.id.id];
}

/*
	Dynamic entries own their whole buffer inside a page. Static entries share one buffer per buffer type and memory visability,
	so the caller still has to add inBufferFirstByte for them.
*/
Buffer& MemoryManager::getBuffer(const MemoryEntry& rEntry_)
{
	if(rEntry_.isForDynamic)
	{
		if(rEntry_.memoryVisability == CPU_SHARED)
		{
			return this->dynamicAllocator.cpuSharedPages[rEntry_.pageId].buffers[rEntry_.bufferId];
		}
		return this->dynamicAllocator.gpuLocalPages[rEntry_.pageId].buffers[rEntry_.bufferId];
	}

	return this->staticAllocator.getBuffer(rEntry_.bufferType, rEntry_.memoryVisability);
}

void MemoryManager::updateEntry(MemoryEntryId entryId_, const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	if(rEntry.locked)
	{
		throw std::runtime_error(std::format("Memory Manager: Entry {} is locked (readOnly), it can't be updated.", rEntry.name));
	}

	/*
		An entry with copies is only written into its shadow, whatever its visability (syncCopiesToFrame moves it on).
		Every other GPU_LOCAL update goes through a staging range. The bytes are written into the range right away, only the copy is deferred until the entry has no upload in flight (for static entries that includes the initial upload).
		CPU_SHARED entries, static or dynamic, are written directly.
	*/
	if(rEntry.hasShadow)
	{
		this->writeShared(rEntry, inEntryOffset_, static_cast<const char*>(data_) + inSrcOffset_, size_);
	}
	else if(rEntry.memoryVisability == GPU_LOCAL)
	{
		StagingDataContainer container = this->getStagingContainer(entryId_, inEntryOffset_, size_);
		container.write(inEntryOffset_, static_cast<const char*>(data_) + inSrcOffset_, size_);
	}
	else
	{
		rEntry.upload(data_, inSrcOffset_, inEntryOffset_, size_); 
	}
} 

/*
	Creates an entry that nothing is uploaded to. Unlike reserveEntry it takes no staging range and needs no commitEntry: the entry has nothing pending, so its buffer can be bound (e.g. into a descriptor set) once it exists.
	A dynamic entry's buffer exists right away. A static entry only gets its place in the static heap here (it is added before StaticAllocator::allocateAndUpload, which skips it because its data is nullptr), so its buffer exists after that call.
	A GPU_LOCAL entry's bytes are undefined until the first updateEntry / getStagingContainer write has been copied.
	A READ_ONLY entry (access.accessMode) can be locked later (lockEntry). Writes stay legal until then.
*/
MemoryEntryId MemoryManager::allocateEntry(const MemoryEntryCreateParams& rParams_)
{
	rParams_.validate();
	if(rParams_.data != nullptr)
	{
		throw std::runtime_error(std::format("Memory Manager: Entry {} has data, allocateEntry uploads nothing. Use addEntry.", rParams_.name));
	}

	MemoryEntryId entryId;
	if(rParams_.allocatorType == STATIC_ALLOCATOR)
	{
		entryId = { STATIC_ALLOCATOR, this->staticAllocator.addEntry(rParams_.name, nullptr, rParams_.size, rParams_.memoryVisability, rParams_.bufferType) };
	}
	else
	{
		entryId = { DYNAMIC_ALLOCATOR, this->dynamicAllocator.allocateEntry(rParams_.name, rParams_.size, rParams_.memoryVisability, rParams_.bufferType) };
	}
	this->getEntry(entryId).readOnly = rParams_.access.isReadOnly();
	this->createCopies(entryId, rParams_.access);
	return entryId;
}

/*
	Creates a dynamic entry without data. The caller writes through getStagingContainer / getSharedContainer and then calls commitEntry.
	A GPU_LOCAL entry counts as pending (OpenBatchId is bigger than any completed batch) until commitEntry queues its copy, so it isn't drawn with garbage.
*/
MemoryEntryId MemoryManager::reserveEntry(const MemoryEntryCreateParams& rParams_)
{
	rParams_.validate();
	if(rParams_.data != nullptr)
	{
		throw std::runtime_error(std::format("Memory Manager: Entry {} has data, reserveEntry takes the data through its containers. Use addEntry.", rParams_.name));
	}
	if(rParams_.allocatorType == STATIC_ALLOCATOR)
	{
		throw std::runtime_error(std::format("Memory Manager: Can't reserve {}. Static entries are uploaded from their data pointer in StaticAllocator::allocateAndUpload.", rParams_.name));
	}

	/* An entry with copies is written into its shadow, so it takes no whole-entry staging range and has no pending copy: its copies are filled by syncCopiesToFrame once it is committed. */
	bool hasCopies = rParams_.access.hasShadow();
	PoolId id;
	if(hasCopies)
	{
		id = this->dynamicAllocator.allocateEntry(rParams_.name, rParams_.size, rParams_.memoryVisability, rParams_.bufferType);
	}
	else
	{
		id = this->dynamicAllocator.reserveEntry(rParams_.name, rParams_.size, rParams_.memoryVisability, rParams_.bufferType);
	}
	MemoryEntry& rEntry = this->dynamicAllocator.memoryEntries[id];
	rEntry.awaitingCommit = true;
	rEntry.readOnly = rParams_.access.isReadOnly();
	if(rParams_.memoryVisability == GPU_LOCAL && !hasCopies)
	{
		rEntry.uploadBatchId = OpenBatchId;
	}
	this->createCopies({ DYNAMIC_ALLOCATOR, id }, rParams_.access);
	return { DYNAMIC_ALLOCATOR, id };
}

void MemoryManager::commitEntry(MemoryEntryId entryId_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	#ifdef ENGINE_DEBUG
		if(!rEntry.awaitingCommit)
		{
			throw std::runtime_error(std::format("Memory Manager: Entry {} is not reserved or was already committed.", rEntry.name));
		}
	#endif

	rEntry.awaitingCommit = false;
	if(rEntry.memoryVisability == GPU_LOCAL && !rEntry.hasShadow)
	{
		rEntry.queueStagedCopy(rEntry.stagingData, 0, rEntry.size);
	}
}

/*
	Marks a readOnly entry as locked: from now on every write path (updateEntry, getStagingContainer, getSharedContainer) throws. Readbacks and removal stay allowed.
	Only an entry declared readOnly at creation can be locked.
*/
void MemoryManager::lockEntry(MemoryEntryId entryId_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	if(!rEntry.readOnly)
	{
		throw std::runtime_error(std::format("Memory Manager: Can't lock entry {}, it was not created readOnly.", rEntry.name));
	}
	rEntry.locked = true;
}

/*
	The container takes entry byte offsets, so Layout::setMemberScalar can pass its entry offset straight to write(). MemoryManager::writeStaging turns that into stagingOffset + offset - stagingData.inEntryOffset,
	which stays inside the range as long as the write stays inside [inEntryOffset_, inEntryOffset_ + size_).
	A reserved entry that wasn't committed yet hands out its whole-entry range, so initial data needs no copy of its own. Otherwise a new open range is allocated and an UPDATE operation is queued for it.
	The caller writes the range right away. The copy is queued by resolvePendingOperations once the entry has no upload in flight, and the range is freed after that batch completes.
	Throws if the entry is locked.
*/
StagingDataContainer MemoryManager::getStagingContainer(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	if(rEntry.locked)
	{
		throw std::runtime_error(std::format("Memory Manager: Entry {} is locked (readOnly), it can't be written.", rEntry.name));
	}
	#ifdef ENGINE_DEBUG
		if(rEntry.memoryVisability != GPU_LOCAL || rEntry.hasShadow)
		{
			throw std::runtime_error(std::format("Memory Manager: Entry {} is CPU_SHARED or has copies, use getSharedContainer.", rEntry.name));
		}
		if(inEntryOffset_ + size_ > rEntry.size)
		{
			throw std::runtime_error(std::format("Memory Manager: Range of {} bytes at offset {} doesn't fit into entry {}.", size_, inEntryOffset_, rEntry.name));
		}
	#endif

	StagingData stagingData;
	if(rEntry.awaitingCommit)
	{
		stagingData = rEntry.stagingData;
	}
	else
	{
		stagingData = this->stagingAllocator.allocate(UPLOAD, size_);
		stagingData.inEntryOffset = inEntryOffset_;
		PoolId opId = this->entriesPendingForUpdate.add(entryId_, stagingData);
		this->pendingOperationsInOrder.emplace_back(UPDATE, opId);
	}

	StagingDataContainer container;
	container.stagingData = stagingData;
	container.validFirstOffset = stagingData.inEntryOffset;
	container.validSize = stagingData.size;
	return container;
}

/* The container only names the entry, so it is persistent and valid as long as the entry exists. Throws if the entry is locked, since the container is for writing (readbacks build their own). */
SharedDataContainer MemoryManager::getSharedContainer(MemoryEntryId entryId_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	if(rEntry.locked)
	{
		throw std::runtime_error(std::format("Memory Manager: Entry {} is locked (readOnly), it can't be written.", rEntry.name));
	}
	#ifdef ENGINE_DEBUG
		if(rEntry.memoryVisability != CPU_SHARED && !rEntry.hasShadow)
		{
			throw std::runtime_error(std::format("Memory Manager: Entry {} is GPU_LOCAL without copies, use getStagingContainer.", rEntry.name));
		}
	#endif

	SharedDataContainer container;
	container.isShared = true;
	container.entryId = entryId_;
	container.validSize = rEntry.size;
	return container;
}


/*
	GPU_LOCAL: allocates a READBACK range and queues a READBACK operation, so the copy is ordered after any earlier UPDATE of the entry and waits for uploads in flight.
	CPU_SHARED: nothing to copy, the request is ready immediately and reads the mapped memory.
	An entry with copies: nothing to copy either, the request is ready immediately and reads the shadow (see readShared).
*/
ReadbackId MemoryManager::requestReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_)
{
	MemoryEntry& rEntry = this->getEntry(entryId_);
	#ifdef ENGINE_DEBUG
		if(rEntry.awaitingCommit)
		{
			throw std::runtime_error(std::format("Memory Manager: Entry {} is reserved and was not committed, there is nothing to read back.", rEntry.name));
		}
		if(inEntryOffset_ + size_ > rEntry.size)
		{
			throw std::runtime_error(std::format("Memory Manager: Readback of {} bytes at offset {} doesn't fit into entry {}.", size_, inEntryOffset_, rEntry.name));
		}
	#endif

	StagingData stagingData = {};
	uint64_t batchId = 0;
	bool copiesToStaging = rEntry.memoryVisability == GPU_LOCAL && !rEntry.hasShadow;
	if(copiesToStaging)
	{
		stagingData = this->stagingAllocator.allocate(READBACK, size_);
		batchId = OpenBatchId;
	}

	ReadbackId readbackId = this->readbackRequests.add(entryId_, stagingData, batchId, inEntryOffset_, size_);
	if(copiesToStaging)
	{
		this->pendingOperationsInOrder.emplace_back(READBACK_COPY, readbackId);
	}
	return readbackId;
}

/* The CPU may read only after the batch that holds the copy was observed completed (its fence signaled). OpenBatchId means the copy is not queued yet. */
bool MemoryManager::isReadbackReady(ReadbackId readbackId_)
{
	return this->isBatchCompleted(this->readbackRequests[readbackId_].batchId);
}

/*
	The container takes entry byte offsets, same as getStagingContainer: the range stores inEntryOffset of the request. READBACK memory is coherent when the device has a coherent+cached type, otherwise readStaging invalidates it.
	The CPU_SHARED container is built here and not through getSharedContainer, so readbacks of locked entries work.
*/
ReadbackDataContainer MemoryManager::getReadbackContainer(ReadbackId readbackId_)
{
	ReadbackRequest& rRequest = this->readbackRequests[readbackId_];
	MemoryEntry& rEntry = this->getEntry(rRequest.memoryEntryId);
	#ifdef ENGINE_DEBUG
		if(!this->isReadbackReady(readbackId_))
		{
			throw std::runtime_error(std::format("Memory Manager: Readback of entry {} is not ready, check isReadbackReady first.", rEntry.name));
		}
	#endif

	ReadbackDataContainer container;
	if(rEntry.memoryVisability == CPU_SHARED || rEntry.hasShadow)
	{
		container.isShared = true;
		container.entryId = rRequest.memoryEntryId;
	}
	else
	{
		container.stagingData = rRequest.stagingData;
		container.stagingData.inEntryOffset = rRequest.inEntryOffset;
	}
	container.validFirstOffset = rRequest.inEntryOffset;
	container.validSize = rRequest.size;
	return container;
}

/* Frees the range. Only allowed once the copy finished, otherwise the GPU could still write into a range that gets reused. */
void MemoryManager::releaseReadback(ReadbackId readbackId_)
{
	#ifdef ENGINE_DEBUG
		if(!this->isReadbackReady(readbackId_))
		{
			throw std::runtime_error("Memory Manager: Can't release a readback before it is ready.");
		}
	#endif

	ReadbackRequest& rRequest = this->readbackRequests[readbackId_];
	const MemoryEntry& rEntry = this->getEntry(rRequest.memoryEntryId);
	if(rEntry.memoryVisability == GPU_LOCAL && !rEntry.hasShadow)
	{
		this->stagingAllocator.releaseReadback(rRequest.stagingData);
	}
	this->readbackRequests.remove(readbackId_);
}

StagingHeap& MemoryManager::getStagingHeap(const StagingData& rStagingData_)
{
	return this->stagingAllocator.getHeap(rStagingData_);
}

/*
	The staging range starts at stagingOffset in its heap and holds the bytes from stagingData.inEntryOffset on (0 for initial uploads, the first updated byte for updates),
	so byte N of the entry is staged at stagingOffset + N - inEntryOffset. Heaps are mapped for good (MemoryBlock::pMapped), and UPLOAD heaps are coherent, so no flush is needed after the write.
*/
void MemoryManager::writeStaging(const StagingData& rStagingData_, uint64_t inEntryOffset_, const void* pSrc_, uint64_t size_)
{
	StagingHeap& rHeap = this->getStagingHeap(rStagingData_);
	uint64_t position = rStagingData_.stagingOffset + inEntryOffset_ - rStagingData_.inEntryOffset;
	std::memcpy(rHeap.memoryBlock.pMapped + position, pSrc_, size_);
}

/*
	Non-coherent memory has to be invalidated before the host reads what the GPU wrote (vkInvalidateMappedMemoryRanges). The offset must be a multiple of nonCoherentAtomSize and so must the size,
	unless the range reaches the end of the allocation (VUID-VkMappedMemoryRange-size-01390, -offset-00687), so the range is widened to atom boundaries and clamped to the block.
*/
void MemoryManager::readStaging(const StagingData& rStagingData_, uint64_t inEntryOffset_, void* pDst_, uint64_t size_)
{
	StagingHeap& rHeap = this->getStagingHeap(rStagingData_);
	uint64_t position = rStagingData_.stagingOffset + inEntryOffset_ - rStagingData_.inEntryOffset;
	if(!rHeap.memoryBlock.isCoherent)
	{
		VkDeviceSize atomSize = getMainDevice().properties.limits.nonCoherentAtomSize;

		VkMappedMemoryRange range = {};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = rHeap.memoryBlock.get();
		range.offset = position - position % atomSize;
		range.size = std::min(alignUp(position + size_, atomSize), rHeap.memoryBlock.size) - range.offset;
		vkInvalidateMappedMemoryRanges(getMainDevice().logicalDevice, 1, &range);
	}
	std::memcpy(pDst_, rHeap.memoryBlock.pMapped + position, size_);
}

/* CPU_SHARED memory is host-coherent, so writes need no flush and reads no invalidate. */
void MemoryManager::writeShared(MemoryEntry& rEntry_, uint64_t inEntryOffset_, const void* pSrc_, uint64_t size_)
{
	/* An entry with copies is only written into its shadow, so a write never touches memory a frame in flight reads. syncCopiesToFrame moves it into the frame's copy. */
	if(rEntry_.hasShadow)
	{
		std::memcpy(rEntry_.shadow.data() + inEntryOffset_, pSrc_, size_);
		rEntry_.version++;
		return;
	}

	/* A single buffer that waits (CUSTOM_COPY_COUNT_WITH_WAIT with a count of 1) is only written once no frame in flight reads it: the fences are the write-after-read dependency. Waiting on a signaled fence is cheap, so repeated writes cost little. */
	if(rEntry_.waitsBeforeWrite)
	{
		for(uint32_t frame = 0; frame < Globals::renderer.framesAtFlightCount; frame++)
		{
			Globals::renderer.passesGraph.waitForFrame(frame);
		}
	}
	std::memcpy(this->getSharedPointer(rEntry_, inEntryOffset_), pSrc_, size_);
}

/* An entry with copies is read from its shadow: the latest written bytes, whichever frame's copy they have reached. */
void MemoryManager::readShared(const MemoryEntry& rEntry_, uint64_t inEntryOffset_, void* pDst_, uint64_t size_)
{
	if(rEntry_.hasShadow)
	{
		std::memcpy(pDst_, rEntry_.shadow.data() + inEntryOffset_, size_);
		return;
	}
	std::memcpy(pDst_, this->getSharedPointer(rEntry_, inEntryOffset_), size_);
}

/* A dynamic entry owns a buffer bound inside a page's block, a static one lives inside the CPUSharedHeap block at its absolute byte. */
char* MemoryManager::getSharedPointer(const MemoryEntry& rEntry_, uint64_t inEntryOffset_)
{
	if(rEntry_.isForDynamic)
	{
		CPUSharedPage& rPage = this->dynamicAllocator.cpuSharedPages[rEntry_.pageId];
		return rPage.memoryBlocks[rEntry_.memoryBlockId].pMapped + rPage.buffersFirstByteOffset[rEntry_.bufferId] + inEntryOffset_;
	}
	return this->staticAllocator.cpuSharedHeap.memoryBlock.pMapped + rEntry_.inGPUFirstByte + inEntryOffset_;
}

void MemoryManager::removeDynamicEntry(MemoryEntryId entryId_)
{
	#ifdef ENGINE_DEBUG
		if(entryId_.allocatorType == STATIC_ALLOCATOR)
		{
			throw std::runtime_error("Memory Manager: Cant remove statically allocated memory entry."); 	
		}
	#endif 

	

	MemoryEntry& rMemoryEntry = this->dynamicAllocator.memoryEntries[entryId_.id]; 

	/* The copies stop being synced right away. */
	std::erase_if(this->copiedEntryIds, [&](const MemoryEntryId& rId_) { return rId_.id == entryId_.id; });

	/* Only GPU_LOCAL entries have uploads in flight that must finish before the buffer is destroyed, so each of them (the copies own buffers too) gets its own deferred REMOVE. CPU_SHARED entries are removed immediately. */
	if(rMemoryEntry.memoryVisability == GPU_LOCAL)
	{
		for(const MemoryEntryId& rCopyId : rMemoryEntry.copyEntryIds)
		{
			PoolId copyRemovalId = this->entriesPendingForRemoval.add(rCopyId);
			this->pendingOperationsInOrder.emplace_back(REMOVE, copyRemovalId);
		}
		PoolId id = this->entriesPendingForRemoval.add(entryId_);
		this->pendingOperationsInOrder.emplace_back(REMOVE, id); 
		return; 
	}

	/* Like the entry itself the copies are removed immediately, so the caller must make sure no frame in flight reads them. */
	for(const MemoryEntryId& rCopyId : rMemoryEntry.copyEntryIds)
	{
		this->dynamicAllocator.removeCPUSharedEntry(rCopyId.id);
	}
	this->dynamicAllocator.removeCPUSharedEntry(entryId_.id);
}

void MemoryManager::destroy()
{
	this->stagingAllocator.deallocate();
	this->staticAllocator.deallocate();
	this->dynamicAllocator.deallocate(); 
}
}
