#pragma once
#include "StagingData.h"
#include "StagingHeap.h"
#include "StagingPage.h"
#include "Utilities.h"
#include <array>
#include <vector>

namespace Graphics
{
/*
	Owns all staging memory: initial uploads (static and dynamic) and updates. Pages are configured with addPage before init(), sorted by the upper bound of the range size.
*/
struct StagingAllocator
{
	/* Both are indexed by StagingDirectionEnum. READBACK pages own no heap until the first readback is requested. */
	std::array<std::vector<StagingPageInfo>, StagingDirectionsCount> pageInfos;
	std::array<std::vector<StagingPage>, StagingDirectionsCount> pages; // Each sorted by smallest upper bound range size -> biggest
	bool initialized = false;

	void addPage(StagingDirectionEnum direction_, uint32_t upperBoundForRangeSize_, StorageUnitEnum upperBoundUnit_, uint32_t heapSize_, StorageUnitEnum heapSizeUnit_, uint32_t maxDormantHeaps_);
	StagingData allocate(StagingDirectionEnum direction_, uint64_t size_);
	void tagBatch(const StagingData& rStagingData_, uint64_t batchId_);
	StagingHeap& getHeap(const StagingData& rStagingData_);
	void releaseCompleted(uint64_t completedBatchId_);
	void releaseReadback(const StagingData& rStagingData_);

	void init();
	void deallocate();
};
}
