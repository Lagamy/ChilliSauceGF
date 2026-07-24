#include "StaticAllocator.h"
#include "Api.h"
#include <cmath>

namespace Graphics
{
UploadId StaticAllocator::addUpload(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum uploadType_) 
{

	this->uploadEntriesGroups[uploadType_].emplace_back(name_, data_, size_, uploadType_, this->gpuHeap.bufferSizes[uploadType_]);
	this->gpuHeap.bufferSizes[uploadType_] += size_; 
	this->stagingHeap.size += size_; 
	return {STATIC, uploadType_, this->uploadEntriesGroups[uploadType_].size() - 1};
}

const UploadEntry& StaticAllocator::getUploadEntry(UploadId id_) 
{
	return this->uploadEntriesGroups[id_.uploadType][id_.id];
}

MemoryBlock& StaticAllocator::getMemoryBlock()
{
	return this->gpuHeap.memory; 
}

Buffer& StaticAllocator::getBuffer(BufferTypeEnum uploadType_)
{
	return this->gpuHeap.buffersPerType[uploadType_];
}

void StaticAllocator::recordCMDs(VkCommandBuffer& cmdBuffer_)
{
	if(!this->allocated)
	{
		throw std::runtime_error("Static Allocator: Can't record CMD Buffer for unallocated memory.");
	}
	size_t memoryBlockOffset = 0; 
	// Info to begin the command buffer record 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
	vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	for(uint32_t i = 0; i < this->uploadEntriesGroups.size(); i++)
	{
		for(auto& rUpload : this->uploadEntriesGroups[i])
		{
			// Since GPUHeap is allocated -> i can now find and save each Uploads first byte position in it. 
			rUpload.inGPUFirstByte = this->gpuHeap.bufferOffsets[i] + rUpload.inBufferFirstByte; 
			void* pStagingMemPoint; // Create an empty typeless pointer.
			vkMapMemory(getMainDevice().logicalDevice, stagingHeap.memoryBlock.get(), memoryBlockOffset + rUpload.inBufferFirstByte, rUpload.size, 0, &pStagingMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
			memcpy(pStagingMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(getMainDevice().logicalDevice, stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		}

		

		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = memoryBlockOffset;
		memoryBlockOffset += this->gpuHeap.bufferSizes[i];		
		bufferCopyRegion.dstOffset = 0; // its buffer local offset 
		bufferCopyRegion.size = this->gpuHeap.bufferSizes[i];

		// Command to copy from srcBuffer to dstBuffer
		vkCmdCopyBuffer(cmdBuffer_, this->stagingHeap.buffer.get(), this->gpuHeap.buffersPerType[i].get(), 1, &bufferCopyRegion);
	}
	vkEndCommandBuffer(cmdBuffer_);
}

void StaticAllocator::allocateAndUpload()
{
	this->stagingHeap.create("Static Staging Heap", VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE);
	this->gpuHeap.create();
	this->allocated = true; 
	this->submitUploads(); 
}

void StaticAllocator::deallocate()
{
	this->gpuHeap.destroy(); 
	this->stagingHeap.destroy();
	this->allocated = false; 
}

void StaticAllocator::create()
{
	this->uploadFinishedSemaphoreId = addSemaphore();
	this->uploadFinishedFenceId = addFence(true); 
	this->uploadPassId = addPass("Static Allocator Uploading",ONESHOT,TRANSFER,this->uploadFinishedFenceId);
	uint32_t taskId = addTaskToPass(this->uploadPassId, "Static Upload", [this](VkCommandBuffer& cmd) { this->recordCMDs(cmd); }); 
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
