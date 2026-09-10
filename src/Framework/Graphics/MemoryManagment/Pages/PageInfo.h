#pragma once 
#include <cstdint> 
#include "Utilities.h"

namespace Graphics 
{
struct PageInfo 
{ 
	uint32_t upperBoundEntrySize;
	uint32_t memoryBlockSize; 
    StorageUnitEnum unit;

    bool operator<(const PageInfo& other) const
    {
        return upperBoundEntrySize < other.upperBoundEntrySize;
    }

    PageInfo(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_, StorageUnitEnum memBlockSizeUnit_) : upperBoundEntrySize(upperBoundEntrySize_), memoryBlockSize(toBytes(memoryBlockSize_, memBlockSizeUnit_)){}; 
};
} 