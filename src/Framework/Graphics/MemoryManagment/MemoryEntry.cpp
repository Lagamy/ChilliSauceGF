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
    void updateStaticGPULocalEntryCMDs(VkCommandBuffer& cmdBuffer_) 
    {
		MemoryManager& rMemoryManager = getMemoryManager(); 
		MemoryEntry& rMemoryEntry = rMemoryManager.staticAllocator.memoryEntries[rMemoryManager.currentEntryToUpload.id];
		StagingHeap& rStagingHeap = rMemoryManager.uploadStagingHeaps[rMemoryEntry.stagingData.heapId];


		// Info to begin the command buffer record 
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
		vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = rMemoryEntry.stagingData.stagingOffset;
		bufferCopyRegion.dstOffset = rMemoryEntry.inGPUFirstByte + rMemoryEntry.inBufferFirstByte + rMemoryEntry.stagingData.inEntryOffset; // its buffer local offset 
		bufferCopyRegion.size = rMemoryEntry.stagingData.size;
		Buffer& rDstBuffer = rMemoryManager.staticAllocator.getBuffer(rMemoryEntry.bufferType); 

		// Command to copy from srcBuffer to dstBuffer
		vkCmdCopyBuffer(cmdBuffer_, rStagingHeap.buffer.get(), rDstBuffer.get(), 1, &bufferCopyRegion);
	    vkEndCommandBuffer(cmdBuffer_);
    }

    void uploadDynamicGPULocalEntryCMDs(VkCommandBuffer& cmdBuffer_)
    {
		MemoryManager& rMemoryManager = getMemoryManager(); 
		MemoryEntry& rMemoryEntry = rMemoryManager.dynamicAllocator.memoryEntries[rMemoryManager.currentEntryToUpload];
		StagingHeap& rStagingHeap = rMemoryManager.uploadStagingHeaps[rMemoryEntry.stagingData.heapId]; 

		
		// Info to begin the command buffer record 
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
		vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = rMemoryEntry.stagingData.stagingOffset;
		bufferCopyRegion.dstOffset = rMemoryEntry.inGPUFirstByte + rMemoryEntry.inBufferFirstByte + rMemoryEntry.stagingData.inEntryOffset; // its buffer local offset 
		bufferCopyRegion.size = rMemoryEntry.stagingData.size;
		Buffer& rDstBuffer = rMemoryManager.dynamicAllocator.gpuLocalPages[rMemoryEntry.pageId].buffers[rMemoryEntry.bufferId];

		// Command to copy from srcBuffer to dstBuffer
		vkCmdCopyBuffer(cmdBuffer_, rStagingHeap.buffer.get(), rDstBuffer.get(), 1, &bufferCopyRegion);
	    vkEndCommandBuffer(cmdBuffer_);
    }

	MemoryEntry::MemoryEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, VkDeviceSize currentBuffSize_) : name(name_), data(data_), size(size_), bufferType(bufferType_), inBufferFirstByte(currentBuffSize_) 
	{
        this->memoryVisability = memoryVisability_; 
		
		if(memoryVisability_ == GPU_ONLY)
		{
			this->isUploadedFenceId = addFence(true); 
    	    this->isUploadedSemaphoreId = addSemaphore();
	    	this->uploadPassId = addPass("Entry Upload", ONESHOT,TRANSFER,this->isUploadedFenceId);
			uint32_t taskId = addTaskToPass(this->uploadPassId, std::format("{} Upload", this->name).c_str(), updateStaticGPULocalEntryCMDs); 
			addWaitSemaphoreToTask(this->uploadPassId, taskId, this->isUploadedSemaphoreId, VK_PIPELINE_STAGE_NONE); 
			addSignalSemaphoreToTask(this->uploadPassId, taskId, this->isUploadedSemaphoreId);
		}
	}; 

    MemoryEntry::MemoryEntry(const char* name_, VkDeviceSize size_,  BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, uint32_t pageId_, PoolId memoryBlockId_, PoolId bufferId_) : name(name_), size(size_), bufferType(bufferType_), pageId(pageId_), memoryBlockId(memoryBlockId_), bufferId(bufferId_) 
    { 
        this->isForDynamic = true; 
        this->memoryVisability = memoryVisability_; 
		
		if(memoryVisability_ == GPU_ONLY)
		{
			this->isUploadedFenceId = addFence(true); 
        	this->isUploadedSemaphoreId = addSemaphore();
	    	this->uploadPassId = addPass("Entry Upload", ONESHOT,TRANSFER,this->isUploadedFenceId);
			uint32_t taskId = addTaskToPass(this->uploadPassId, std::format("{} Upload", this->name).c_str(), uploadDynamicGPULocalEntryCMDs); 
			addWaitSemaphoreToTask(this->uploadPassId, taskId, this->isUploadedSemaphoreId, VK_PIPELINE_STAGE_NONE); 
			addSignalSemaphoreToTask(this->uploadPassId, taskId, this->isUploadedSemaphoreId);				
		}
	};


    bool MemoryEntry::isPendingUpload()
    {
		if(!isForDynamic && getMemoryManager().staticAllocator.uploadsInGPU)
		{
			return true; 
		}	
        VkResult result = vkGetFenceStatus(getMainDevice().logicalDevice, getFence(this->isUploadedFenceId));
        return result == VK_SUCCESS ? false : true;
    }

	void MemoryEntry::upload(const void* data_, uint64_t inSrcOffset_, uint64_t inEntryOffset_, uint64_t size_)
    {
		
		void* pCPUSharedMemPoint; // Create an empty typeless pointer.
        if(this->memoryVisability == CPU_SHARED)
        {

			if(this->isForDynamic)
			{
				CPUSharedPage& page = getMemoryManager().dynamicAllocator.cpuSharedPages[this->pageId]; 
				vkMapMemory(getMainDevice().logicalDevice, page.memoryBlocks[this->memoryBlockId].get(), page.buffersFirstByteOffset[this->memoryBlockId] + inEntryOffset_, size_, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
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
			StagingHeap& rStagingHeap = getMemoryManager().uploadStagingHeaps[this->stagingData.heapId]; 
			vkMapMemory(getMainDevice().logicalDevice, rStagingHeap.memoryBlock.get(), this->stagingData.stagingOffset, size_, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
			memcpy(pCPUSharedMemPoint, static_cast<const char*>(data_) + inSrcOffset_, size_);  // writes to *Staging/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(getMainDevice().logicalDevice, rStagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
			enablePass(this->uploadPassId); 
 	   }
	}
}