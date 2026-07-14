#include "FrameResources.h"
#include "Api.h"

namespace Graphics 
{
	void FrameResources::setup(uint32_t id_)
	{
		std::ostringstream name;
		this->id = id_; 
		this->frameCmdPools.create();
		this->imageAcquireSemaphore.create(); 
		this->frameAvailableFence.create(VK_FENCE_CREATE_SIGNALED_BIT); 
	}


	void FrameResources::destroy()
	{
		this->imageAcquireSemaphore.destroy();
		this->frameAvailableFence.destroy(); 
		this->frameCmdPools.destroy();
	};
}
