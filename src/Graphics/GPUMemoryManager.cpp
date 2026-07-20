#include "GPUMemoryManager.h"
#include "Fence.h"
#include "Api.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

void GPUMemoryManager::create()
{
	this->staticUploadFinishedSemaphoreId = addSemaphore("Upload Finished");
	this->staticUploadFinishedFenceId = addFence("Upload Finished", true); 
	this->staticUploadPassId = addPass("Static Allocator Uploading",ONESHOT,TRANSFER,this->staticUploadFinishedFenceId);
	uint32_t taskId = addTaskToPass(this->staticUploadPassId, "Static Upload", [this](VkCommandBuffer& cmd) { this->staticAllocator.recordCMDs(cmd); }); 
	addSignalSemaphoreToTask(this->staticUploadPassId, taskId, this->staticUploadFinishedSemaphoreId);
}

void GPUMemoryManager::destroy() 
{
	this->staticAllocator.deallocate();
}


void GPUMemoryManager::submitStaticUploads()
{

	vkResetFences(getMainDevice().logicalDevice, 1, &getFence(staticUploadFinishedFenceId));
	enablePass(this->staticUploadPassId); 
}

void GPUMemoryManager::submitUpdateCmdsIfNeeded()
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
