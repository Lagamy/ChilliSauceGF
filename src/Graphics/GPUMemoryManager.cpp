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
	this->staticUploadFinishedSemaphoreId = addSemaphore("Static Upload Finished");
	this->staticUploadFinishedFenceId = addFence("Static Upload Finished", VK_FENCE_CREATE_SIGNALED_BIT); 
	this->staticUploadCmdBufferId = addCmdBufferBlueprint(
		ONESHOT,
		TRANSFER,
		[this](VkCommandBuffer& cmd) { this->staticAllocator.recordCMDs(cmd); } 
	);
}

void GPUMemoryManager::destroy() 
{
	this->staticAllocator.deallocate();
}


void GPUMemoryManager::submitStaticUploadCmds()
{
	Fence& rUploadFinished = getFence(this->staticUploadFinishedFenceId);
	//resetOneShotCmdBuf(TRANSFER, this->staticUploadCmdBufferId, rUploadFinished); 
	recordOneShotCmdBuf(TRANSFER, this->staticUploadCmdBufferId);

	vkResetFences(getMainDevice().logicalDevice, 1, &rUploadFinished.get());
	// Submit command buffer to the Transfer Queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &getCommandBuffer(TRANSFER, ONESHOT, this->staticUploadCmdBufferId);
	submitInfo.signalSemaphoreCount = 1; 
	submitInfo.pSignalSemaphores = &getSemaphore(this->staticUploadFinishedSemaphoreId).vkHandle; 

	// Submit transfer command to transfer Queue and wait till it finishes(Not optimal)
	VkResult result = vkQueueSubmit(getMainDevice().queues.transferQueue, 1, &submitInfo, rUploadFinished.get());
	if(result != VK_SUCCESS)
	{
		//throw std::runtime_error(""); 
		throw std::runtime_error("Static allocator: command submit failed.\n");
		return;
	}
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
}
