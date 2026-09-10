#pragma once 
#include <cstdint> 

struct PageInfo 
{ 
	uint32_t upperBoundEntrySize;
	uint32_t memoryBlockSize; 
    
    bool operator<(const PageInfo& other) const
    {
        return upperBoundEntrySize < other.upperBoundEntrySize;
    }
}; 