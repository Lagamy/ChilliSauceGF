#include "GPUMemoryManager.h"


uint32_t GPUMemoryManager::addEntry(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_) 
{
	return this->memoryEntries.add(PhysicalGPUBuffer(name_, size_, bufferUsageFlags_, bufferSharingMode_, cpuVisible_));
}

void GPUMemoryManager::removeEntry(uint32_t id_) 
{
	this->memoryEntries.remove(id_);
}

void GPUMemoryManager::upload(uint32_t entryId_, const void* data_) // full upload
{
	this->memoryEntries.get(entryId_).upload(data_); 
}

void GPUMemoryManager::upload(uint32_t entryId_, const void* data_, size_t byteAmount_, size_t srcStartingByte_, size_t dstStartingByte_) // partial upload
{
	this->memoryEntries.get(entryId_).upload(data_, byteAmount_, srcStartingByte_, dstStartingByte_); 
}

PhysicalGPUBuffer& GPUMemoryManager::getEntry(uint32_t id_) 
{
	return this->memoryEntries.get(id_); 	
} 

void GPUMemoryManager::destroy() 
{
	for(size_t i = 0; i < this->memoryEntries.objects.size(); i++)
	{
		this->memoryEntries.remove(i); 
	}
}

void UploadEntry::recordCMDs(CommandBuffer& cmdBuffer_)
{
	
	// Inst optimal for loading many meshes, so TODO: optimize it for multiple transfer buffers use + sync via Fences and Semaphores to render only loaded meshes 

	// Info to begin the command buffer record 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 

	// Begin command buffer transfer commands
	vkBeginCommandBuffer(cmdBuffer_.get(), &beginInfo);

	// Region of data to copy from and to 
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = this->srcOffset; // copy whole thing(from the beginning)
	bufferCopyRegion.dstOffset = this->dstOffset;
	bufferCopyRegion.size = this->bufferSize;

	// Command to copy from srcBuffer to dstBuffer 
	vkCmdCopyBuffer(transferCommandBuffer, this->srcBuffer, this->dstBuffer, 1, &bufferCopyRegion);

	vkEndCommandBuffer(transferCommandBuffer);

	// Submit command buffer to the Transfer Queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	// Submit transfer command to transfer Queue and wait till it finishes(Not optimal) 
	vkQueueSubmit(transferQueue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue_); // Code doesn't executes past this line untill _transferQueue is empty. Also prevents creating new CommandBuffer for new mesh before this one is dispatched.(We can have limited amounts of them, otherwise - app will crash) 

	// Free temporary command buffer back to pool(transferCommandBuffer object no longer exists on GPU side)
	vkFreeCommandBuffers(Demo::renderer.mainDevice.logicalDevice, transferCommandPool_, 1, &transferCommandBuffer);
}
