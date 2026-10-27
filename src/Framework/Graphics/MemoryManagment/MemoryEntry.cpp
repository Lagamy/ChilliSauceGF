#include "MemoryEntry.h"
#include "Api.h"
#include "CPUSharedPage.h"
#include "DynamicAllocator.h"
#include "MemoryManager.h"
#include "PoolId.h"
#include "Utilities.h"
#include <limits>
#include <vulkan/vulkan_core.h>



namespace Graphics 
{
	MemoryEntry::MemoryEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, VkDeviceSize currentBuffSize_) : name(name_), data(data_), size(size_), bufferType(bufferType_), inBufferFirstByte(currentBuffSize_) 
	{
        this->memoryVisability = memoryVisability_; 
	}; 

    MemoryEntry::MemoryEntry(const char* name_, VkDeviceSize size_,  BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, uint32_t pageId_, PoolId memoryBlockId_, PoolId bufferId_) : name(name_), size(size_), bufferType(bufferType_), pageId(pageId_), memoryBlockId(memoryBlockId_), bufferId(bufferId_) 
    { 
        this->isForDynamic = true; 
        this->memoryVisability = memoryVisability_; 
	};


    bool MemoryEntry::isPendingUpload()
    {
        return !getMemoryManager().isBatchCompleted(this->uploadBatchId);
    }

    bool MemoryEntry::isPendingReadback()
    {
        return !getMemoryManager().isBatchCompleted(this->readbackBatchId);
    }

	void MemoryEntry::upload(const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
    {
        if(this->memoryVisability == CPU_SHARED)
        {
			getMemoryManager().writeShared(*this, inEntryOffset_, static_cast<const char*>(data_) + inSrcOffset_, size_);
		}
        else
        {
			#ifdef ENGINE_DEBUG
				if(inEntryOffset_ + size_ > this->size)
				{
					throw std::runtime_error(std::format("Memory Entry {}: upload of {} bytes at offset {} doesn't fit into the entry.", this->name, size_, inEntryOffset_));
				}
			#endif

			getMemoryManager().writeStaging(this->stagingData, inEntryOffset_, static_cast<const char*>(data_) + inSrcOffset_, size_);
			this->queueStagedCopy(this->stagingData, inEntryOffset_, size_);
 	   }
	}

	/*
		The staging range is already written. The copy into the GPU Local buffer is only queued here, MemoryManager records and submits all queued copies as one batch.
		The range stays open until queueCopy returns the batch that reads it, then it is tagged with that batch so it is freed only after the batch's fence was observed signaled.
		Static entries live inside one big buffer per type, so their offset in that buffer is added. Dynamic entries own their whole buffer.
		The range holds the bytes from rStagingData_.inEntryOffset on, so byte N of the entry is staged at stagingOffset + N - inEntryOffset.
	*/
	void MemoryEntry::queueStagedCopy(const StagingData& rStagingData_, uint64_t inEntryOffset_, uint64_t size_)
	{
		StagingHeap& rStagingHeap = getMemoryManager().getStagingHeap(rStagingData_);

		PendingCopy pendingCopy = {};
		pendingCopy.srcBuffer = rStagingHeap.buffer.get();
		pendingCopy.srcOffset = rStagingData_.stagingOffset + inEntryOffset_ - rStagingData_.inEntryOffset;
		pendingCopy.size = size_;
		pendingCopy.dstBuffer = getMemoryManager().getBuffer(*this).get();
		if(this->isForDynamic)
		{
			pendingCopy.dstOffset = inEntryOffset_;
		}
		else
		{
			pendingCopy.dstOffset = this->inBufferFirstByte + inEntryOffset_;
		}
		this->uploadBatchId = getMemoryManager().queueCopy(pendingCopy);
		getMemoryManager().stagingAllocator.tagBatch(rStagingData_, this->uploadBatchId);
	}

	/*
		Mirror of queueStagedCopy: the copy runs from the entry's buffer into the readback range, which holds exactly the requested bytes (so its offset is stagingOffset).
		The range is not tagged with the batch. It stays allocated until MemoryManager::releaseReadback, and the returned batch id only tells when the CPU may read it.
		The entry also remembers the batch (readbackBatchId): the copy reads the entry's buffer, so the buffer must not be destroyed before that batch completed.
	*/
	uint64_t MemoryEntry::queueReadbackCopy(const StagingData& rStagingData_, uint64_t inEntryOffset_, uint64_t size_)
	{
		StagingHeap& rStagingHeap = getMemoryManager().getStagingHeap(rStagingData_);

		PendingCopy pendingCopy = {};
		pendingCopy.isReadback = true;
		pendingCopy.srcBuffer = getMemoryManager().getBuffer(*this).get();
		if(this->isForDynamic)
		{
			pendingCopy.srcOffset = inEntryOffset_;
		}
		else
		{
			pendingCopy.srcOffset = this->inBufferFirstByte + inEntryOffset_;
		}
		pendingCopy.dstBuffer = rStagingHeap.buffer.get();
		pendingCopy.dstOffset = rStagingData_.stagingOffset;
		pendingCopy.size = size_;
		this->readbackBatchId = getMemoryManager().queueCopy(pendingCopy);
		return this->readbackBatchId;
	}
}
