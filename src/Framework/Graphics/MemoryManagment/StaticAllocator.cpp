#include "StaticAllocator.h"
#include "Api.h"
#include "Utilities.h"

namespace Graphics
{
void uploadCMDs(VkCommandBuffer& cmdBuffer_)
{
	StaticAllocator& rStaticAllocator = getMemoryManager().staticAllocator; 

	#ifdef ENGINE_DEBUG
		if(!rStaticAllocator.allocated)
		{
			throw std::runtime_error("Static Allocator: Can't record CMD Buffer for unallocated memory.\n");
		}
	#endif
	size_t memoryBlockOffset = 0; 
	// Info to begin the command buffer record 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
	vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	
	void* pCPUSharedMemPoint; // Create an empty typeless pointer.
	for(uint32_t i = 0; i < rStaticAllocator.uploadEntryGroupPerMemVisability[CPU_SHARED].entriesPerBufferType.size(); i++)
	{

		std::vector<UploadEntry> entriesForCurrentBufferType = rStaticAllocator.uploadEntryGroupPerMemVisability[GPU_ONLY].entriesPerBufferType[i];  
		for(auto& rUpload : entriesForCurrentBufferType)
		{
			// Since GPUHeap is allocated -> i can now find and save each Uploads first byte position in it. 
			rUpload.inGPUFirstByte = rStaticAllocator.cpuSharedHeap.bufferOffsets[i] + rUpload.inBufferFirstByte; 
			vkMapMemory(getMainDevice().logicalDevice, rStaticAllocator.cpuSharedHeap.memoryBlock.get(), memoryBlockOffset + rUpload.inBufferFirstByte, rUpload.size, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
			memcpy(pCPUSharedMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *Staging/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(getMainDevice().logicalDevice, rStaticAllocator.stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		}
	}

	for(uint32_t i = 0; i < rStaticAllocator.uploadEntryGroupPerMemVisability[GPU_ONLY].entriesPerBufferType.size(); i++)
	{
		std::vector<UploadEntry> entriesForCurrentBufferType = rStaticAllocator.uploadEntryGroupPerMemVisability[GPU_ONLY].entriesPerBufferType[i];  
		for(auto& rUpload : entriesForCurrentBufferType)
		{
			// Since GPUHeap is allocated -> i can now find and save each Uploads first byte position in it. 
			rUpload.inGPUFirstByte = rStaticAllocator.gpuHeap.bufferOffsets[i] + rUpload.inBufferFirstByte; 
			vkMapMemory(getMainDevice().logicalDevice, rStaticAllocator.stagingHeap.memoryBlock.get(), memoryBlockOffset + rUpload.inBufferFirstByte, rUpload.size, 0, &pCPUSharedMemPoint);
			memcpy(pCPUSharedMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *Staging/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(getMainDevice().logicalDevice, rStaticAllocator.stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		}

		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = memoryBlockOffset;
		memoryBlockOffset += rStaticAllocator.gpuHeap.bufferSizes[i];		
		bufferCopyRegion.dstOffset = 0; // its buffer local offset 
		bufferCopyRegion.size = rStaticAllocator.gpuHeap.bufferSizes[i];

		// Command to copy from srcBuffer to dstBuffer
		vkCmdCopyBuffer(cmdBuffer_, rStaticAllocator.stagingHeap.buffer.get(), rStaticAllocator.gpuHeap.buffers[i].get(), 1, &bufferCopyRegion);
	}
	

	rStaticAllocator.allocated = true; 
	vkEndCommandBuffer(cmdBuffer_);
}


UploadId StaticAllocator::addEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_) 
{
	
	#ifdef ENGINE_DEBUG
		if(this->allocated)
		{
			throw std::runtime_error("Static Allocator: Can't add entry to already allocated Static heaps.");  
		}
	#endif
	
	if(memoryVisability_ == GPU_ONLY)
	{
		this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].emplace_back(name_, data_, size_, uploadType_, this->gpuHeap.bufferSizes[uploadType_]);
		this->gpuHeap.bufferSizes[uploadType_] += size_; 
		this->stagingHeap.size += size_; 
		return {STATIC, memoryVisability_, uploadType_, this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].size() - 1};
	}
	else 
	{
		this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].emplace_back(name_, data_, size_, uploadType_);
		this->cpuSharedHeap.bufferSizes[uploadType_] += size_; 
		this->cpuSharedHeap.size += size_; 
		return {STATIC, memoryVisability_, uploadType_, this->uploadEntryGroupPerMemVisability[memoryVisability_][uploadType_].size() - 1};
	}
}

void StaticAllocator::updateEntry(UploadId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t byteAmmount_) 
{

	#ifdef ENGINE_DEBUG
		if(!allocated)
		{
			throw std::runtime_error("Static Allocator: Can't update entry before allocation.");
		}
	#endif

}

const UploadEntry& StaticAllocator::getEntry(UploadId id_) 
{
	return this->uploadEntryGroupPerMemVisability[id_.memoryVisability][id_.uploadType][id_.id];
}

MemoryBlock& StaticAllocator::getGPUMemoryBlock()
{
	return this->gpuHeap.memoryBlock; 
}

Buffer& StaticAllocator::getBuffer(BufferTypeEnum uploadType_)
{
	return this->gpuHeap.buffers[uploadType_];
}

void StaticAllocator::allocateAndUpload()
{
	this->cpuSharedHeap.create(); 
	this->stagingHeap.create("Static Staging Heap");
	this->gpuHeap.create();
	this->submitUploads(); 
}

void StaticAllocator::deallocate()
{
	if(this->allocated)
	{
		this->gpuHeap.destroy(); 
		this->stagingHeap.destroy();
		this->cpuSharedHeap.destroy(); 
		this->allocated = false; 
	}
}

void StaticAllocator::create()
{
	this->uploadFinishedSemaphoreId = addSemaphore();
	this->uploadFinishedFenceId = addFence(true); 
	this->uploadPassId = addPass("Static Allocator Uploading",ONESHOT,TRANSFER,this->uploadFinishedFenceId);
	uint32_t taskId = addTaskToPass(this->uploadPassId, "Static Upload", uploadCMDs); 
	addSignalSemaphoreToTask(this->uploadPassId, taskId, this->uploadFinishedSemaphoreId);
	
}

void StaticAllocator::submitUploads()
{
	vkResetFences(getMainDevice().logicalDevice, 1, &getFence(uploadFinishedFenceId));
	enablePass(this->uploadPassId); 
}

void StaticAllocator::checkUploadsStatus()
{
	if(!this->uploadsInGPU)
	{
		if(wasFenceSignaled(this->uploadFinishedFenceId))
		{
			this->uploadsInGPU = true; 
		}
	} 
}
}


