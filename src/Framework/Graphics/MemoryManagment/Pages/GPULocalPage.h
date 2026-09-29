#pragma once 
#include "StagingHeap.h"
#include "BasePage.h"

namespace Graphics
{
struct GPULocalPage : BasePage 
{

	std::vector<PoolId> freeStagingHeaps; // It creates 
	
	void destroy();
	void init();

	GPULocalPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_);
};
}
