#include "QueueTransferSync.h"
#include "Api.h"

namespace Graphics
{
void QueueTransferSync::setup()
{
	/*
		Equal family indices are the same VkQueue (Device takes queue 0 of each family), so nothing needs to be shared or synchronized between them.
		Otherwise a VK_SHARING_MODE_EXCLUSIVE buffer may be used by one queue family at a time unless ownership is transferred (spec: Resource Sharing Mode, Queue Family Ownership Transfer).
		VK_SHARING_MODE_CONCURRENT lets the listed families use it without ownership transfers, but the accesses still have to be synchronized, which the done semaphores do.
	*/
	const QueueFamilyIndices& rFamilies = getMainDevice().queueFamilyIndices;
	this->separateFamilies = false;
	for(uint8_t family = GRAPHICS; family <= COMPUTE; family++)
	{
		this->ownerFamily[family] = (QueueFamilyEnum)family;
		for(int8_t other = family - 1; other >= GRAPHICS; other--)
		{
			if(rFamilies.indices[other] == rFamilies.indices[family])
			{
				this->ownerFamily[family] = this->ownerFamily[other];
			}
		}
		this->separateFamilies = this->separateFamilies || this->ownerFamily[family] != GRAPHICS;
	}
	this->gpuLocalSharingMode = this->separateFamilies ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

	if(this->separateFamilies)
	{
		for(uint8_t family = GRAPHICS; family <= COMPUTE; family++)
		{
			if(this->ownerFamily[family] == family)
			{
				this->doneSemaphoreIds[family] = addSemaphore();
			}
		}
	}
}

/*
	A semaphore wait operation is the only ordering between two queues (spec: Synchronization and Cache Control -> Semaphores): the signal's first scope covers everything submitted to its queue before it,
	the wait's second scope covers everything submitted after it on the waiting queue, and both make memory accesses available and visible, so no queue family ownership transfer or cross-queue barrier is needed.
	The wait stage is ALL_COMMANDS, which every queue family supports. A done semaphore that is still signaled has to be waited on before it is signaled again, so a submit also waits on its own queue's one.
*/
void QueueTransferSync::addWaits(std::vector<VkSemaphore>& rWaitSemaphores_, std::vector<VkPipelineStageFlags>& rWaitStages_)
{
	for(uint8_t family = GRAPHICS; family <= COMPUTE; family++)
	{
		if(this->separateFamilies && this->doneSignaled[family])
		{
			rWaitSemaphores_.push_back(getSemaphore(this->doneSemaphoreIds[family]));
			rWaitStages_.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			this->doneSignaled[family] = false;
		}
	}
}

void QueueTransferSync::addSignal(QueueFamilyEnum queueFamily_, std::vector<VkSemaphore>& rSignalSemaphores_)
{
	if(this->separateFamilies)
	{
		QueueFamilyEnum owner = this->ownerFamily[queueFamily_];
		rSignalSemaphores_.push_back(getSemaphore(this->doneSemaphoreIds[owner]));
		this->doneSignaled[owner] = true;
	}
}
}
