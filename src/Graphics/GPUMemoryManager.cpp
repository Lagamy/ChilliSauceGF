#include "GPUMemoryManager.h"
#include "Fence.h"
#include "Api.h"
#include "MemoryBlock.h"
#include "SubmissionBatchId.h"
#include "Utilities.h"
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

void GPUMemoryManager::create()
{
	this->staticUploadFinishedSemaphore.create();
	this->staticUploadFinishedFence.create(VK_FENCE_CREATE_SIGNALED_BIT); 
	this->staticUploadCmdBufferId = addCmdBufferBlueprint(
		ONESHOT,
		TRANSFER,
		[this](VkCommandBuffer& cmd) { this->staticAllocator.recordCMDs(cmd); } 
	);
}

void GPUMemoryManager::destroy() 
{
	this->staticUploadFinishedSemaphore.destroy(); 
	this->staticUploadFinishedFence.destroy();
	this->staticAllocator.deallocate();
}


void GPUMemoryManager::submitStaticUploadCMDs()
{
	//resetOneShotCmdBuf(TRANSFER, this->staticUploadCmdBufferId, rUploadFinished); 
	recordOneShotCmdBuf(TRANSFER, this->staticUploadCmdBufferId);

	vkResetFences(getMainDevice().logicalDevice, 1, &staticUploadFinishedFence.get());
	// Submit command buffer to the Transfer Queue
	SubmissionBatchId batchId = addSubmissionBatch("Static Allocator Uploading", TRANSFER); 
	PoolId submitionId = addSubmission("Static Allocator Upload", batchId, &getCommandBuffer(TRANSFER, ONESHOT, this->staticUploadCmdBufferId), 1);
	addSignalSemaphoreToSubmission(batchId, submitionId, this->staticUploadFinishedSemaphore.get());

	// Submit transfer command to transfer Queue and wait till it finishes(Not optimal)
	submitToTransferQueue(batchId, staticUploadFinishedFence.get());
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
