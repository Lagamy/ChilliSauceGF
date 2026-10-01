#pragma once 
#include <cstdint> 
#include "Utilities.h"

namespace Graphics 
{
struct PageInfo 
{ 
	VkDeviceSize upperBoundEntrySize;
	VkDeviceSize memoryBlockSize; 
	uint32_t maxDormantStagingHeaps;

    bool operator<(const PageInfo& other) const
    {
        return upperBoundEntrySize < other.upperBoundEntrySize;
    }

    PageInfo(VkDeviceSize upperBoundEntrySize_, VkDeviceSize memoryBlockSize_, uint32_t maxDormantStagingHeaps_) : upperBoundEntrySize(upperBoundEntrySize_), memoryBlockSize(memoryBlockSize_), maxDormantStagingHeaps(maxDormantStagingHeaps_){}; 
};
} 