#pragma once
#include "PoolNameless.h"
#include "StagingHeap.h"
#include "GPULocalHeap.h"
#include <vulkan/vulkan.h>

namespace Graphics
{

struct GPUPage
{
	StagingHeap stagingHeap;
	GPULocalHeap gpuHeap = GPULocalHeap(false);

	PoolNameless<memoryInterval> freeMemIntervals;  // Use buffers in GPU heap as a base. Check if adjecent buffers exist to morf continious free mem intervals into 1 big one   

	uint32_t upperBoundEntrySize; 
	GPUPage(uint32_t upperBoundEntrySize_);
};
}
