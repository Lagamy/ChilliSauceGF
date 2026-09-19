#include "MemoryManager.h"
#include "Fence.h"
#include "Api.h"
#include "MemoryBlock.h"
#include "UploadId.h"
#include "Utilities.h"
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

void MemoryManager::setup()
{
	this->staticAllocator.create(); 
	this->dynamicAllocator.init();
}

void MemoryManager::destroy() 
{
	this->staticAllocator.deallocate();
	this->dynamicAllocator.deallocate(); 
}

void MemoryManager::checkUploadsStatus()
{
	this->staticAllocator.checkUploadsStatus(); 
}

void MemoryManager::submitUpdateCmdsIfNeeded()
{
	// if(this->updateNeeded)
	// {
	// 	// Submit command buffer to the Transfer Queue
	// 	VkSubmitInfo submitInfo = {};
	// 	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	// 	submitInfo.commandBufferCount = 1;
	// 	submitInfo.pCommandBuffers = &getCommandBuffer(TRANSFER, ONESHOT, 0);

	// 	// Submit transfer command to transfer Queue and wait till it finishes(Not optimal) 
	// 	vkQueueSubmit(getMainDevice().queues.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

	// 	// Free temporary command buffer back to pool(transferCommandBuffer object no longer exists on GPU side)
	// 	vkFreeCommandBuffers(getMainDevice().logicalDevice, getCommandPool(TRANSFER, ONESHOT), 1, &getCommandBuffer(TRANSFER, ONESHOT, this->staticUploadCmdBufferId));
	// 	this->updateNeeded = false;
	// }
}
}
