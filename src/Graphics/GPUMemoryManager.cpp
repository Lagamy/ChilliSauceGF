#include "GPUMemoryManager.h"
#include "Fence.h"
#include "Api.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include <limits>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

void GPUMemoryManager::create()
{
	this->staticUploadFinishedSemaphoreId = addSemaphore("Static Upload Finished");
	this->staticUploadFinishedFenceId = addFence("Static Upload Finished", VK_FENCE_CREATE_SIGNALED_BIT); 
	this->staticUploadCmdBufferId = addCmdBufferBlueprint(
		ONESHOT,
		TRANSFER, 
		[this](VkCommandBuffer& cmd) { this->staticAllocator.recordCMDs(cmd); } 
	);

	// this->updateCmdBufferId = addCmdBufferBlueprint(
		// ONESHOT,
		// TRANSFER, 
		// [this](VkCommandBuffer& cmd) { recordUpdatesCMDs(cmd); } 
	// );
}

void GPUMemoryManager::destroy() 
{
	this->staticAllocator.deallocate();
}


// void recordUpdatesCMDs(VkCommandBuffer& cmdBuffer_)
// {
	// Info to begin the command buffer record 
	// VkCommandBufferBeginInfo beginInfo = {};
	// beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 
	// vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	// Begin command buffer transfer commands
	// GPUMemoryEntry& rMemoryEntry = Demo::renderer.gpuMemoryManager.getEntry(rUploadEntry.entryId); 
		
	// Region of data to copy from and to 
	// VkBufferCopy bufferCopyRegion = {};
	// bufferCopyRegion.srcOffset = 0; 
	// bufferCopyRegion.dstOffset = rUploadEntry.dstStartingByte;
	// bufferCopyRegion.size = rUploadEntry.partialUpload ? rUploadEntry.byteAmount : rMemoryEntry.size;

	// Command to copy from srcBuffer to dstBuffer 
	// vkCmdCopyBuffer(cmdBuffer_, this->stagingHeap.memoryEntry.buffer.get(), rMemoryEntry.buffer.get(), 1, &bufferCopyRegion);
	// vkEndCommandBuffer(cmdBuffer_);
// }

void GPUMemoryManager::submitStaticUploadCmds()
{

	Fence& rUploadFinished = getFence(this->staticUploadFinishedFenceId);
	vkWaitForFences(getMainDevice().logicalDevice, 1, &rUploadFinished.get(), VK_TRUE, std::numeric_limits<uint64_t>::max()); // Block this CPU thread until the GPU signals this fence.
	// Free temporary command buffer back to pool(transferCommandBuffer object will no longer exist on GPU side)
	vkFreeCommandBuffers(getMainDevice().logicalDevice, getCommandPool(TRANSFER, ONESHOT), 1, &getCommandBuffer(TRANSFER, ONESHOT, this->staticUploadCmdBufferId));

	vkResetFences(getMainDevice().logicalDevice, 1, &rUploadFinished.get());
	// Submit command buffer to the Transfer Queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &getCommandBuffer(TRANSFER, ONESHOT, 0);
	submitInfo.signalSemaphoreCount = 1; 
	submitInfo.pSignalSemaphores = &getSemaphore(this->staticUploadFinishedSemaphoreId).vkHandle; 

	// Submit transfer command to transfer Queue and wait till it finishes(Not optimal)
	vkQueueSubmit(getMainDevice().queues.transferQueue, 1, &submitInfo, rUploadFinished.get());
}

void GPUMemoryManager::submitUpdateCmdsIfNeeded()
{
	if(this->updateNeeded)
	{
		// Submit command buffer to the Transfer Queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &getCommandBuffer(TRANSFER, ONESHOT, 0);

		// Submit transfer command to transfer Queue and wait till it finishes(Not optimal) 
		vkQueueSubmit(getMainDevice().queues.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

		// Free temporary command buffer back to pool(transferCommandBuffer object no longer exists on GPU side)
		vkFreeCommandBuffers(getMainDevice().logicalDevice, getCommandPool(TRANSFER, ONESHOT), 1, &getCommandBuffer(TRANSFER, ONESHOT, this->staticUploadCmdBufferId));
		this->updateNeeded = false;
	}
}

UploadEntry::UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, size_t currentBuffSize_) 
{
	this->name = name_;
	this->data = data_; 
	this->heapStartingByte = currentBuffSize_ + 1;
	this->size = size_; 
}

// UpdateEntry::UpdateEntry(PoolId entryId_, const void* data_, size_t heapStartinByte_)
// {
	// this->heapStartingByte = heapStartinByte_;
	// this->entryId = entryId_; 
	// this->data = data_; 
// }

// UpdateEntry::UpdateEntry(PoolId entryId_, const void* data_, size_t heapStartinByte_, size_t byteAmount_, size_t dstStartingbyte_)
// {
	// this->entryId = entryId_; 
	// this->data = data_; 
	// this->byteAmount = byteAmount_; 
	// this->heapStartingByte = heapStartinByte_; 
	// this->dstStartingByte = dstStartingbyte_;
// }
}
