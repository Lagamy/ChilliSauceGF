#include "MemoryManager.h"
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

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

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
	vkEndCommandBuffer(cmdBuffer_);
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

	this->uploadBatches.resize(1);
	for(uint32_t i = 0; i < this->uploadBatches.size(); i++)
	{
		UploadBatch& rBatch = this->uploadBatches[i];
		rBatch.fenceId = addFence(true);
		rBatch.passId = addPass(std::format("Memory Manager Upload Batch {}", i).c_str(), ONESHOT, TRANSFER, rBatch.fenceId);
		uint32_t taskId = addTaskToPass(rBatch.passId, "Queued Copies Upload", uploadQueuedCopiesCMDs);
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


MemoryEntryId MemoryManager::addEntry(const char* name_, const void* data_, VkDeviceSize size_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_, bool readOnly_)
{
	MemoryEntryId entryId;
	if(allocatorType_ == STATIC)
	{
		entryId = { STATIC, this->staticAllocator.addEntry(name_, data_, size_, memoryVisability_, uploadType_) };
	}
	else
	{
		entryId = { DYNAMIC, this->dynamicAllocator.addAndUploadEntry(name_, data_, size_, memoryVisability_, uploadType_) };
	}
	this->getEntry(entryId).readOnly = readOnly_;
	return entryId;
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
	if(entryId_.allocatorType == DYNAMIC)
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
		Every GPU_LOCAL update goes through a staging range. The bytes are written into the range right away, only the copy is deferred until the entry has no upload in flight (for static entries that includes the initial upload).
		CPU_SHARED entries, static or dynamic, are written directly.
	*/
	if(rEntry.memoryVisability == GPU_LOCAL)
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
	readOnly_ declares that the entry can be locked later (lockEntry). Writes stay legal until then.
*/
MemoryEntryId MemoryManager::allocateEntry(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum bufferType_, VkDeviceSize size_, bool readOnly_)
{
	MemoryEntryId entryId;
	if(allocatorType_ == STATIC)
	{
		entryId = { STATIC, this->staticAllocator.addEntry(name_, nullptr, size_, memoryVisability_, bufferType_) };
	}
	else
	{
		entryId = { DYNAMIC, this->dynamicAllocator.allocateEntry(name_, size_, memoryVisability_, bufferType_) };
	}
	this->getEntry(entryId).readOnly = readOnly_;
	return entryId;
}

/*
	Creates a dynamic entry without data. The caller writes through getStagingContainer / getSharedContainer and then calls commitEntry.
	A GPU_LOCAL entry counts as pending (OpenBatchId is bigger than any completed batch) until commitEntry queues its copy, so it isn't drawn with garbage.
*/
MemoryEntryId MemoryManager::reserveEntry(const char* name_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum bufferType_, VkDeviceSize size_, bool readOnly_)
{
	if(allocatorType_ == STATIC)
	{
		throw std::runtime_error(std::format("Memory Manager: Can't reserve {}. Static entries are uploaded from their data pointer in StaticAllocator::allocateAndUpload.", name_));
	}

	PoolId id = this->dynamicAllocator.reserveEntry(name_, size_, memoryVisability_, bufferType_);
	MemoryEntry& rEntry = this->dynamicAllocator.memoryEntries[id];
	rEntry.awaitingCommit = true;
	rEntry.readOnly = readOnly_;
	if(memoryVisability_ == GPU_LOCAL)
	{
		rEntry.uploadBatchId = OpenBatchId;
	}
	return { DYNAMIC, id };
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
	if(rEntry.memoryVisability == GPU_LOCAL)
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
	The container takes entry byte offsets, so Layout::setMember can pass its entry offset straight to write(). MemoryManager::writeStaging turns that into stagingOffset + offset - stagingData.inEntryOffset,
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
		if(rEntry.memoryVisability != GPU_LOCAL)
		{
			throw std::runtime_error(std::format("Memory Manager: Entry {} is CPU_SHARED, use getSharedContainer.", rEntry.name));
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
		if(rEntry.memoryVisability != CPU_SHARED)
		{
			throw std::runtime_error(std::format("Memory Manager: Entry {} is GPU_LOCAL, use getStagingContainer.", rEntry.name));
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
	if(rEntry.memoryVisability == GPU_LOCAL)
	{
		stagingData = this->stagingAllocator.allocate(READBACK, size_);
		batchId = OpenBatchId;
	}

	ReadbackId readbackId = this->readbackRequests.add(entryId_, stagingData, batchId, inEntryOffset_, size_);
	if(rEntry.memoryVisability == GPU_LOCAL)
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
	if(rEntry.memoryVisability == CPU_SHARED)
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
	if(this->getEntry(rRequest.memoryEntryId).memoryVisability == GPU_LOCAL)
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
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(getMainDevice().physicalDevice, &deviceProperties);
		VkDeviceSize atomSize = deviceProperties.limits.nonCoherentAtomSize;

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
void MemoryManager::writeShared(const MemoryEntry& rEntry_, uint64_t inEntryOffset_, const void* pSrc_, uint64_t size_)
{
	std::memcpy(this->getSharedPointer(rEntry_, inEntryOffset_), pSrc_, size_);
}

void MemoryManager::readShared(const MemoryEntry& rEntry_, uint64_t inEntryOffset_, void* pDst_, uint64_t size_)
{
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
		if(entryId_.allocatorType == STATIC)
		{
			throw std::runtime_error("Memory Manager: Cant remove statically allocated memory entry."); 	
		}
	#endif 

	

	MemoryEntry& rMemoryEntry = this->dynamicAllocator.memoryEntries[entryId_.id]; 

	/* Only GPU_LOCAL entries have uploads in flight that must finish before the buffer is destroyed. CPU_SHARED entries are removed immediately. */
	if(rMemoryEntry.memoryVisability == GPU_LOCAL)
	{
		PoolId id = this->entriesPendingForRemoval.add(entryId_);
		this->pendingOperationsInOrder.emplace_back(REMOVE, id); 
		return; 
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
