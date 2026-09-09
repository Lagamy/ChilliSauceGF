#pragma once 
#include "Utilities.h"
#include "PoolNameless.h"
#include "PoolId.h"

namespace Graphics 
{
struct FreeSpace
{
	PoolNameless<MemoryInterval> memoryIntervals;
	std::vector<PoolId> aliveIntervals; // For quicker itteration. Self erases if is filled up in (Add buffer dynamic function)  
    std::unordered_map<uint64_t, PoolId> intervalByFirst;
	std::unordered_map<uint64_t, PoolId> intervalByLast; 
};  
}