#pragma once
#include "PoolNameless.h"
#include "StagingHeap.h"
#include "GPULocalHeap.h"
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPULocalPage
{
	uint32_t upperBoundEntrySize; 
	StagingHeap stagingHeap;
	GPULocalHeap heap = GPULocalHeap(false);
	PoolNameless<MemoryInterval> freeMemIntervals;  // Use buffers in GPU heap as a base. Check if adjecent buffers exist to morf continious free mem intervals into 1 big one   
	std::vector<uint32_t> aliveFreeMemIntervals; // I need to Use Pool on memIntervals for id stability, and this to avoid itterating through non alive freeMemIntervals 
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;

	GPULocalPage(uint32_t upperBoundEntrySize_);
	void addBuffer(); 
	void removeBuffer();
	void init();
};
}
