#pragma once
#include "PoolId.h"
#include "Utilities.h"
#include <array>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
/*
	Everything that depends on the graphics, transfer and compute queue families being the same family or not. Device prefers the graphics family for all three, so separate families are the fallback.
	GPU_LOCAL buffers are written by the transfer queue and read by the graphics and compute queues, so when the families differ they are VK_SHARING_MODE_CONCURRENT.
	Other buffers (CPU_SHARED, staging) are only used by one family and stay EXCLUSIVE.
	Queues don't order work against each other, so with separate families every submit signals its queue's "done" semaphore and waits for the done semaphores of all queues that submitted since (see addWaits / addSignal).
*/
struct QueueTransferSync
{
	bool separateFamilies = false;
	VkSharingMode gpuLocalSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	/* Families that share a queue family index share one done semaphore, owned by the lowest QueueFamilyEnum of them. */
	std::array<QueueFamilyEnum, 3> ownerFamily = { GRAPHICS, TRANSFER, COMPUTE };
	/* Indexed by QueueFamilyEnum: graphicsDone, transferDone, computeDone. Only owners have one. */
	std::array<PoolId, 3> doneSemaphoreIds = { UninitializedPoolId, UninitializedPoolId, UninitializedPoolId };
	/* A done semaphore is signaled and not waited on yet. A binary semaphore has to be waited on before it is signaled again. */
	std::array<bool, 3> doneSignaled = { false, false, false };

	void setup(); // after the device and the SyncManager exist, before any GPU_LOCAL buffer is created
	void addWaits(std::vector<VkSemaphore>& rWaitSemaphores_, std::vector<VkPipelineStageFlags>& rWaitStages_); // for the first submit of a batch
	void addSignal(QueueFamilyEnum queueFamily_, std::vector<VkSemaphore>& rSignalSemaphores_); // for the last submit of a batch
};
}
