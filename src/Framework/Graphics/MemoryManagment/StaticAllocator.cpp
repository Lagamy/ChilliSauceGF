#include "StaticAllocator.h"
#include "Api.h"
#include "Utilities.h"
#include <cmath>

namespace Graphics
{

void staticUploadCMDs(VkCommandBuffer& cmdBuffer_)
{
	StaticAllocator& rStaticAllocator = getMemoryManager().staticAllocator; 
	if(!rStaticAllocator.allocated)
	{
		throw std::runtime_error("Static Allocator: Can't record CMD Buffer for unallocated memory.");
	}
	size_t memoryBlockOffset = 0; 
	// Info to begin the command buffer record 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
	vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	for(uint32_t i = 0; i < rStaticAllocator.gpuUploadEntryGroupPerBufferType.size(); i++)
	{
		for(auto& rUpload : rStaticAllocator.gpuUploadEntryGroupPerBufferType[i])
		{
			// Since GPUHeap is allocated -> i can now find and save each Uploads first byte position in it. 
			rUpload.inGPUFirstByte = rStaticAllocator.gpuHeap.bufferOffsets[i] + rUpload.inBufferFirstByte; 
			void* pStagingMemPoint; // Create an empty typeless pointer.
			vkMapMemory(getMainDevice().logicalDevice, rStaticAllocator.gpuStagingHeap.memoryBlock.get(), memoryBlockOffset + rUpload.inBufferFirstByte, rUpload.size, 0, &pStagingMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
			memcpy(pStagingMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(getMainDevice().logicalDevice, rStaticAllocator.gpuStagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		}

		

		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = memoryBlockOffset;
		memoryBlockOffset += rStaticAllocator.gpuHeap.bufferSizes[i];		
		bufferCopyRegion.dstOffset = 0; // its buffer local offset 
		bufferCopyRegion.size = rStaticAllocator.gpuHeap.bufferSizes[i];

		// Command to copy from srcBuffer to dstBuffer
		vkCmdCopyBuffer(cmdBuffer_, rStaticAllocator.gpuStagingHeap.buffer.get(), rStaticAllocator.gpuHeap.buffersPerType[i].get(), 1, &bufferCopyRegion);
	}
	vkEndCommandBuffer(cmdBuffer_);
}


UploadId StaticAllocator::addUpload(const char* name_, const void* data_, MemoryVisabilityEnum memoryVisability_, VkDeviceSize size_, BufferTypeEnum uploadType_) 
{
	if(memoryVisability_ == DEVICE_LOCAL)
	{
		this->gpuUploadEntryGroupPerBufferType[uploadType_].emplace_back(name_, data_, size_, uploadType_, this->gpuHeap.bufferSizes[uploadType_]);
		this->gpuHeap.bufferSizes[uploadType_] += size_; 
		this->gpuStagingHeap.size += size_; 
		return {STATIC, memoryVisability_, uploadType_, this->gpuUploadEntryGroupPerBufferType[uploadType_].size() - 1};
	}
	else 
	{
		this->cpuSharedUploadEntryGroupsPerBufferType[uploadType_].emplace_back(name_, data_, size_, uploadType_, 0);
		return {STATIC, memoryVisability_, uploadType_, this->cpuSharedUploadEntryGroupsPerBufferType[uploadType_].size() - 1};
	}
}

const UploadEntry& StaticAllocator::getUploadEntry(UploadId id_) 
{
	return this->gpuUploadEntryGroupPerBufferType[id_.uploadType][id_.id];
}

MemoryBlock& StaticAllocator::getMemoryBlock()
{
	return this->gpuHeap.memory; 
}

Buffer& StaticAllocator::getBuffer(BufferTypeEnum uploadType_)
{
	return this->gpuHeap.buffersPerType[uploadType_];
}

void StaticAllocator::allocateAndUpload()
{
	this->gpuStagingHeap.create("Static Staging Heap", VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE);
	this->gpuHeap.create();
	this->allocated = true; 
	this->submitUploads(); 
}

void StaticAllocator::deallocate()
{
	this->gpuHeap.destroy(); 
	this->gpuStagingHeap.destroy();
	this->allocated = false; 
}

void StaticAllocator::create()
{
	this->uploadFinishedSemaphoreId = addSemaphore();
	this->uploadFinishedFenceId = addFence(true); 
	this->uploadPassId = addPass("Static Allocator Uploading",ONESHOT,TRANSFER,this->uploadFinishedFenceId);
	uint32_t taskId = addTaskToPass(this->uploadPassId, "Static Upload", staticUploadCMDs); 
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


