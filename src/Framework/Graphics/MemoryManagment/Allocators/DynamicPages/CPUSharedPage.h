#pragma once 
#include "CPUSharedHeap.h"

namespace Graphics
{
struct CPUSharedPage
{
	uint32_t upperBoundEntrySize;
	CPUSharedHeap cpuSharedHeap; 
	
	CPUSharedPage(uint32_t upperBoundEntrySize_); 
};
}
