#pragma once
#include "Utilities.h"
#include "Pool.h"
#include "PoolId.h"

namespace Graphics
{
struct FreeSpace
{
	Pool<MemoryInterval> memoryIntervals;
	std::vector<PoolId> aliveIntervals; // For quicker itteration. Self erases if is filled up in (Add buffer dynamic function)
    std::unordered_map<uint64_t, PoolId> intervalByFirst;
	std::unordered_map<uint64_t, PoolId> intervalByLast;

	void init(uint64_t size_); // One free interval covering [0, size_ - 1]
	bool allocate(uint64_t size_, uint64_t& rOffset_); // First fit, no alignment.
	void free(uint64_t firstByte_, uint64_t lastByte_); // Merges with touching intervals
	void destroy();
};
}
