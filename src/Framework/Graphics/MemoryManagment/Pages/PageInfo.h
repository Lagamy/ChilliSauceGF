#pragma once 
#include <cstdint> 
#include "Utilities.h"

namespace Graphics 
{
struct PageInfo 
{ 
	VkDeviceSize upperBoundEntrySize;
	VkDeviceSize memoryBlockSize; 

    bool operator<(const PageInfo& other) const
    {
        return upperBoundEntrySize < other.upperBoundEntrySize;
    }

    PageInfo(VkDeviceSize upperBoundEntrySize_, VkDeviceSize memoryBlockSize_) : upperBoundEntrySize(upperBoundEntrySize_), memoryBlockSize(memoryBlockSize_){}; 
};
} 