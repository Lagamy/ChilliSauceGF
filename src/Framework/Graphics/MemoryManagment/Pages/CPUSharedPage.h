#pragma once 
#include "CPUSharedHeap.h"
#include "PoolNameless.h"
#include "Utilities.h"

namespace Graphics
{
struct CPUSharedPage
{
	uint32_t upperBoundEntrySize;
	CPUSharedHeap heap = CPUSharedHeap(false); 
	PoolNameless<MemoryInterval> freeMemIntervals;  // Use buffers in GPU heap as a base. Check if adjecent buffers exist to morf continious free mem intervals into 1 big one   
	std::vector<uint32_t> aliveFreeMemIntervals; // I need to Use Pool on memIntervals for id stability, and this to avoid itterating through non alive freeMemIntervals 
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;

	CPUSharedPage(uint32_t upperBoundEntrySize_);
	void addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBuffer(PoolId bufferId_); // Adds free Mem Intervals
	void init(); 
};
}
