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
		if(!isForDynamic && !getMemoryManager().staticAllocator.staticUploadCompleted)
		{
			return true; 
		}	
        return this->uploadBatchId > getMemoryManager().completedUploadBatchId;
    }

	void MemoryEntry::upload(const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
    {
		
		void* pCPUSharedMemPoint; // Create an empty typeless pointer.
        if(this->memoryVisability == CPU_SHARED)
        {

			if(this->isForDynamic)
			{
				CPUSharedPage& page = getMemoryManager().dynamicAllocator.cpuSharedPages[this->pageId]; 
				vkMapMemory(getMainDevice().logicalDevice, page.memoryBlocks[this->memoryBlockId].get(), page.buffersFirstByteOffset[this->bufferId] + inEntryOffset_, size_, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
				memcpy(pCPUSharedMemPoint, static_cast<const char*>(data_) + inSrcOffset_, size_);  
				vkUnmapMemory(getMainDevice().logicalDevice, page.memoryBlocks[memoryBlockId].get()); 
			}
			else 
			{
				vkMapMemory(getMainDevice().logicalDevice, getMemoryManager().staticAllocator.cpuSharedHeap.memoryBlock.get(), this->inGPUFirstByte + inEntryOffset_, size_, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
				memcpy(pCPUSharedMemPoint, static_cast<const char*>(data_) + inSrcOffset_, size_);  // writes to *Staging/Shared memory in Ram* via CPU pointer
				vkUnmapMemory(getMainDevice().logicalDevice, getMemoryManager().staticAllocator.stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
			}
		}
        else
        {
			#ifdef ENGINE_DEBUG
				if(inEntryOffset_ + size_ > this->size)
				{
					throw std::runtime_error(std::format("Memory Entry {}: upload of {} bytes at offset {} doesn't fit into the entry.", this->name, size_, inEntryOffset_));
				}
			#endif

			/* 
				The staging heap mirrors the memory block. stagingOffset is where the entry's buffer starts in the block, so byte N of the entry is staged at stagingOffset + N. 
				The copy reads from the same position, which keeps staged bytes of different parts of one entry from overwriting each other. 
			*/
			uint64_t stagingPosition = this->stagingData.stagingOffset + inEntryOffset_; 
			StagingHeap& rStagingHeap = getMemoryManager().uploadStagingHeaps[this->stagingData.heapId]; 
			vkMapMemory(getMainDevice().logicalDevice, rStagingHeap.memoryBlock.get(), stagingPosition, size_, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
			memcpy(pCPUSharedMemPoint, static_cast<const char*>(data_) + inSrcOffset_, size_);  // writes to *Staging/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(getMainDevice().logicalDevice, rStagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data

			/*
				Staging memory is written, the copy into the GPU Local buffer is only queued here. MemoryManager records and submits all queued copies as one batch.
				Static entries live inside one big buffer per type, so their offset in that buffer is added. Dynamic entries own their whole buffer.
			*/
			PendingCopy pendingCopy = {};
			pendingCopy.srcBuffer = rStagingHeap.buffer.get();
			pendingCopy.srcOffset = stagingPosition;
			pendingCopy.size = size_;
			if(this->isForDynamic)
			{
				pendingCopy.dstBuffer = getMemoryManager().dynamicAllocator.gpuLocalPages[this->pageId].buffers[this->bufferId].get();
				pendingCopy.dstOffset = inEntryOffset_;
			}
			else
			{
				pendingCopy.dstBuffer = getMemoryManager().staticAllocator.getBuffer(this->bufferType).get();
				pendingCopy.dstOffset = this->inBufferFirstByte + inEntryOffset_;
			}
			this->uploadBatchId = getMemoryManager().queueCopy(pendingCopy);
 	   }
	}
}