#include "FrameResources.h"
#include "Api.h"

namespace Graphics 
{
	void FrameResources::setup(uint32_t id_)
	{
		std::ostringstream name;
		this->id = id_; 
		this->frameCmdPools.create();
		name << "Frame " << id_ << " Available"; 
		this->swapchainImageAvailableSemaphoreId = addSemaphore(name.str().c_str());
		this->frameAvailableFenceId = addFence(name.str().c_str(), VK_FENCE_CREATE_SIGNALED_BIT); 
	}
}
