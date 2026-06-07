#include "GPUMemoryManager.h"
#include "Globals.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>


PoolId GPUMemoryManager::addEntry(const char* name_, VkDeviceSize size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_) 
{
	return this->memoryEntries.add(GPUMemoryEntry(name_, size_, BYTE, bufferUsageFlags_, bufferSharingMode_, cpuVisible_), name_);
}

void GPUMemoryManager::removeEntry(PoolId id_) 
{
	this->memoryEntries.remove(id_);
}

void GPUMemoryManager::upload(PoolId entryId_, const void* data_) // full upload
{
	
	GPUMemoryEntry& rMemoryEntry = this->memoryEntries.get(entryId_); 
	// Map our vertex data to vertex Buffer 
	memcpy(rMemoryEntry.pCpuSharedData, data_, rMemoryEntry.size);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer

	
	// Map our vertex data to vertex Buffer 
	this->gpuLocalUploadEntries.emplace_back(entryId_, data_);
}

void GPUMemoryManager::upload(PoolId entryId_, const void* data_, size_t byteAmount_, size_t srcStartingByte_, size_t dstStartingByte_) // partial upload
{
	GPUMemoryEntry& rMemoryEntry = this->memoryEntries.get(entryId_); 
	

	// Map our vertex data to vertex Buffer 
	void* sharedDataP; // Create an empty typeless pointer.
	vkMapMemory(Demo::renderer.mainDevice.logicalDevice, uploadHeap.memoryEntry.memoryBlock.get(), uploadHeap.lastByte, byteAmount_, 0, &sharedDataP);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
	memcpy(sharedDataP, static_cast<const char*>(data_) + srcStartingByte_, byteAmount_);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer
	vkUnmapMemory(Demo::renderer.mainDevice.logicalDevice, uploadHeap.memoryEntry.memoryBlock.get());	// Unmap vertexBufferMemory from data
	this->uploadHeap.lastByte += byteAmount_ + 1; // + 1 - so next upload would start in unocupied memory cell
	// Copy staging buffer to vertex buffer on GPU
	this->gpuLocalUploadEntries.emplace_back(entryId_, data_, byteAmount_, this->uploadHeap.lastByte, dstStartingByte_);
}

GPUMemoryEntry& GPUMemoryManager::getEntry(PoolId id_) 
{
	return this->memoryEntries.get(id_); 	
} 

void GPUMemoryManager::create()
{
	// Create Upload Heap
	this->uploadHeap.memoryEntry = GPUMemoryEntry("Upload Heap", 256, MB, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, true);

	Demo::renderer.demoManager.addCmdBufferBlueprint(
		ONESHOT,
		TRANSFER, 
		[this](VkCommandBuffer& cmd) { recordCMDs(cmd); } 
	);
}

void GPUMemoryManager::destroy() 
{
	this->memoryEntries.clear();	
}

void GPUMemoryManager::recordCMDs(VkCommandBuffer& cmdBuffer_)
{
	// Inst optimal for loading many meshes, so TODO: optimize it for multiple transfer buffers use + sync via Fences and Semaphores to render only loaded meshes 

	// Info to begin the command buffer record 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 
	vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	for(auto& rUploadEntry : this->gpuLocalUploadEntries)
	{
		// Begin command buffer transfer commands
		GPUMemoryEntry& rMemoryEntry = Demo::renderer.gpuMemoryManager.getEntry(rUploadEntry.entryId); 
		
		// Region of data to copy from and to 
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = 0; 
		bufferCopyRegion.dstOffset = rUploadEntry.dstStartingByte;
		bufferCopyRegion.size = rUploadEntry.partialUpload ? rUploadEntry.byteAmount : rMemoryEntry.size;

		// Command to copy from srcBuffer to dstBuffer 
		vkCmdCopyBuffer(cmdBuffer_, this->uploadHeap.buffer.get(), rMemoryEntry.buffer.get(), 1, &bufferCopyRegion);
	}

	vkEndCommandBuffer(cmdBuffer_);
	this->gpuLocalUploadEntries.clear();
}

void GPUMemoryManager::submitTransferOps()
{
	// Submit command buffer to the Transfer Queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &Demo::renderer.getCommandBuffer(TRANSFER, ONESHOT, 0);

	// Submit transfer command to transfer Queue and wait till it finishes(Not optimal) 
	vkQueueSubmit(Demo::renderer.mainDevice.queues.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(Demo::renderer.mainDevice.queues.transferQueue); // Code doesn't executes past this line untill _transferQueue is empty. Also prevents creating new CommandBuffer for new mesh before this one is dispatched.(We can have limited amounts of them, otherwise - app will crash) 

	// Free temporary command buffer back to pool(transferCommandBuffer object no longer exists on GPU side)
	vkFreeCommandBuffers(Demo::renderer.mainDevice.logicalDevice, transferCommandPool_, 1, &transferCommandBuffer);
}

UploadEntry::UploadEntry(PoolId entryId_, const void* data_)
{
	this->entryId = entryId_; 
	this->data = data_; 
}

UploadEntry::UploadEntry(PoolId entryId_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_)
{
	this->entryId = entryId_; 
	this->data = data_; 
	this->byteAmount = byteAmount_; 
	this->srcStartingByte = srcStartingbyte_; 
	this->dstStartingByte = dstStartingbyte_;
}


