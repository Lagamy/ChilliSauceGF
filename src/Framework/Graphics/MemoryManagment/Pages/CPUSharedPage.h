#pragma once 
#include "CPUSharedHeap.h"
#include "PoolNameless.h"

namespace Graphics
{
struct CPUSharedPage
{
	uint32_t upperBoundEntrySize;
	CPUSharedHeap cpuSharedHeap = CPUSharedHeap(false); 
	PoolNameless<memoryInterval> freeMemIntervals;  // Use buffers in GPU heap as a base. Check if adjecent buffers exist to morf continious free mem intervals into 1 big one   
	
	CPUSharedPage(uint32_t upperBoundEntrySize_); 
};
}
