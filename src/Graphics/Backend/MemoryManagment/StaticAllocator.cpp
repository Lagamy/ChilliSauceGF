#include "StaticAllocator.h"
#include "Api.h"
#include <stdexcept>

namespace Graphics
{
uint32_t StaticAllocator::addUpload(const char* name_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_) 
{
	this->uploadEntriesGroups[uploadType_].emplace_back(name_, data_, size_, this->gpuHeap.bufferSizes[uploadType_]);
	this->gpuHeap.bufferSizes[uploadType_] += size_; 
	return this->uploadEntriesGroups.size();
}

const UploadEntry& StaticAllocator::getUploadEntry(uint32_t id_, UploadTypeEnum uploadType_) 
{
	return this->uploadEntriesGroups[uploadType_][id_];
}

MemoryBlock& StaticAllocator::getMemoryBlock()
{
	return this->gpuHeap.memory; 
}

Buffer& StaticAllocator::getBuffer(UploadTypeEnum uploadType_)
{
	return this->gpuHeap.buffersPerType[uploadType_];
}

void StaticAllocator::recordCMDs(VkCommandBuffer& cmdBuffer_)
{
	if(!this->allocated)
	{
		throw std::runtime_error("Static Allocator: Can't record CMD Buffer for unallocated memory.");
	}
	size_t offset = 0; 
	// Info to begin the command buffer record 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
	vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	for(uint32_t i = 0; i < this->uploadEntriesGroups.size(); i++)
	{
		for(auto& rUpload : this->uploadEntriesGroups[i])
		{
			void* pStagingMemPoint; // Create an empty typeless pointer.
			vkMapMemory(Demo::renderer.mainDevice.logicalDevice, stagingHeap.memoryBlock.get(), rUpload.heapStartingByte, rUpload.size, 0, &pStagingMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
			memcpy(pStagingMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer
			vkUnmapMemory(Demo::renderer.mainDevice.logicalDevice, stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		}

		

		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = offset;
		offset += this->gpuHeap.bufferSizes[i];
		bufferCopyRegion.dstOffset = offset - 1;
		bufferCopyRegion.size = this->gpuHeap.bufferSizes[i];

		// Command to copy from srcBuffer to dstBuffer
		vkCmdCopyBuffer(cmdBuffer_, this->stagingHeap.buffer.get(), this->gpuHeap.buffersPerType[i].get(), 1, &bufferCopyRegion);
	}
	vkEndCommandBuffer(cmdBuffer_);
}

void StaticAllocator::allocate()
{
	this->stagingHeap = GPUMemoryEntry("Static Staging Heap", this->gpuHeap.size, BYTE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, true);
	this->gpuHeap.create();
	this->allocated = true; 
}

void StaticAllocator::deallocate()
{
	this->gpuHeap.destroy(); 
	this->stagingHeap.destroy();
	this->allocated = false; 
}
}
