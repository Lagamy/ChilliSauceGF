#pragma once 
#include "CPUSharedHeap.h"
#include "PoolId.h"
#include "PoolNameless.h"
#include "Utilities.h"
#include "FreeSpace.h"
#include <unordered_map>

namespace Graphics
{
struct CPUSharedPage
{
	uint32_t upperBoundEntrySize;
	CPUSharedHeap heap = CPUSharedHeap(false);
	std::vector<FreeSpace> freeSpacePerBlock;  
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;

	CPUSharedPage(uint32_t upperBoundEntrySize_);
	void addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBuffer(PoolId bufferId_); // Adds free Mem Intervals
	void init();
};
}