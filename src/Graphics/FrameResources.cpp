#include "FrameResources.h"
#include "Api.h"

namespace Graphics 
{
	void FrameResources::setup()
	{
		this->frameCmdPools.create();
		this->frameAvailableSemaphoreId = addSemaphore("Frame Available");
		this->frameAvailableFenceId = addFence("Frame Available", VK_FENCE_CREATE_SIGNALED_BIT); 
		this->frameFinishedSemaphoreId = addSemaphore("Frame Finished");
	}
}
