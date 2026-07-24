#include "FrameResources.h"
#include "Api.h"

namespace Graphics 
{
	void FrameResources::setup(uint32_t id_)
	{
		std::ostringstream name;
		this->id = id_; 
		this->imageAcquiredSemaphoreId = addSemaphore();
		this->frameAvailableFenceId = addFence(true);
	}

	void FrameResources::createCmdPools()
	{
		this->frameCmdPools.create();
	}

	void FrameResources::destroy()
	{
		this->frameCmdPools.destroy();
	};
}
