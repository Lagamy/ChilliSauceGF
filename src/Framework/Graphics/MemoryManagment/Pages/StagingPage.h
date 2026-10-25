#pragma once
#include "Pool.h"
#include "PoolId.h"
#include "FreeSpace.h"
#include "StagingHeap.h"
#include "StagingData.h"
#include "Utilities.h"
#include <vector>

namespace Graphics
{
/* One slice of a staging heap holding one write. It is open (OpenBatchId) until its copy is queued, and free again once the upload batch it was queued into completed. */
struct StagingRange
{
	PoolId heapId;
	uint64_t firstByte;
	uint64_t lastByte;
	uint64_t batchId;
};

/* Configuration of one StagingPage, sorted by the biggest range it serves. */
struct StagingPageInfo
{
	uint64_t upperBoundRangeSize;
	uint64_t heapSize;
	uint32_t maxDormantHeaps;

	bool operator<(const StagingPageInfo& other) const
	{
		return upperBoundRangeSize < other.upperBoundRangeSize;
	}

	StagingPageInfo(uint64_t upperBoundRangeSize_, uint64_t heapSize_, uint32_t maxDormantHeaps_) : upperBoundRangeSize(upperBoundRangeSize_), heapSize(heapSize_), maxDormantHeaps(maxDormantHeaps_){}
};

struct StagingPage
{
	StagingDirectionEnum direction;
	uint64_t upperBoundRangeSize;
	uint64_t heapSize;
	uint32_t maxDormantHeaps;

	Pool<StagingHeap> heaps;
	Pool<FreeSpace> freeSpacePerHeap; // Same ids as heaps.
	std::vector<PoolId> aliveHeaps; // In creation order.
	std::vector<StagingRange> liveRanges; // In allocation order. Batch ids are stamped later, so they are not sorted.

	StagingRange allocate(uint64_t size_);
	void tagBatch(PoolId heapId_, uint64_t firstByte_, uint64_t batchId_);
	void releaseCompleted(uint64_t completedBatchId_);
	void release(PoolId heapId_, uint64_t firstByte_);
	void destroyExcessDormantHeaps();
	void destroy();

	StagingPage(StagingDirectionEnum direction_, uint64_t upperBoundRangeSize_, uint64_t heapSize_, uint32_t maxDormantHeaps_);
};
}
