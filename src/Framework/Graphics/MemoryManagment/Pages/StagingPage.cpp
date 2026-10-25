#include "StagingPage.h"
#include <stdexcept>

namespace Graphics
{
	StagingRange StagingPage::allocate(uint64_t size_)
	{
		uint64_t offset = 0;
		PoolId heapId = UninitializedPoolId;

		for(PoolId aliveHeapId : this->aliveHeaps)
		{
			if(this->freeSpacePerHeap[aliveHeapId].allocate(size_, offset))
			{
				heapId = aliveHeapId;
				break;
			}
		}

		/* No heap has a big enough free interval. The page upper bound is never bigger than a heap, so a new heap always fits the range. */
		if(heapId == UninitializedPoolId)
		{
			heapId = this->heaps.add(this->heapSize, this->direction);
			PoolId freeSpaceId = this->freeSpacePerHeap.add();
			#ifdef ENGINE_DEBUG
				if(freeSpaceId != heapId)
				{
					throw std::runtime_error("Staging Page: StagingHeap and its FreeSpace ended up with different ids.");
				}
			#endif
			this->freeSpacePerHeap[freeSpaceId].init(this->heapSize);
			this->aliveHeaps.emplace_back(heapId);

			if(!this->freeSpacePerHeap[freeSpaceId].allocate(size_, offset))
			{
				throw std::runtime_error("Staging Page: Range doesn't fit into a new staging heap.");
			}
		}

		this->heaps[heapId].pendingUploadsCount++;
		StagingRange range = { heapId, offset, offset + size_ - 1, OpenBatchId };
		this->liveRanges.emplace_back(range);
		return range;
	}

	void StagingPage::tagBatch(PoolId heapId_, uint64_t firstByte_, uint64_t batchId_)
	{
		/* The range was allocated recently, so the search starts from the newest. */
		for(uint32_t i = this->liveRanges.size(); i > 0; i--)
		{
			StagingRange& rRange = this->liveRanges[i - 1];
			if(rRange.heapId == heapId_ && rRange.firstByte == firstByte_)
			{
				rRange.batchId = batchId_;
				return;
			}
		}
		throw std::runtime_error("Staging Page: Can't tag a range that is not live.");
	}

	void StagingPage::releaseCompleted(uint64_t completedBatchId_)
	{
		/* READBACK ranges are not freed here. The CPU still has to read them after their batch completes, so only StagingPage::release frees them. */
		if(this->direction == READBACK)
		{
			return;
		}

		/* The batch fence is signaled, so the copies stopped reading these ranges. Open ranges have OpenBatchId and are never completed. Ranges are not sorted by batch id, so every one is checked. */
		uint32_t keptCount = 0;
		bool freedAny = false;
		for(uint32_t i = 0; i < this->liveRanges.size(); i++)
		{
			StagingRange& rRange = this->liveRanges[i];
			if(rRange.batchId <= completedBatchId_)
			{
				this->freeSpacePerHeap[rRange.heapId].free(rRange.firstByte, rRange.lastByte);
				this->heaps[rRange.heapId].pendingUploadsCount--;
				freedAny = true;
			}
			else
			{
				this->liveRanges[keptCount] = rRange;
				keptCount++;
			}
		}
		if(!freedAny)
		{
			return;
		}
		this->liveRanges.resize(keptCount);
		this->destroyExcessDormantHeaps();
	}

	void StagingPage::release(PoolId heapId_, uint64_t firstByte_)
	{
		for(uint32_t i = 0; i < this->liveRanges.size(); i++)
		{
			StagingRange& rRange = this->liveRanges[i];
			if(rRange.heapId == heapId_ && rRange.firstByte == firstByte_)
			{
				this->freeSpacePerHeap[rRange.heapId].free(rRange.firstByte, rRange.lastByte);
				this->heaps[rRange.heapId].pendingUploadsCount--;
				this->liveRanges.erase(this->liveRanges.begin() + i);
				this->destroyExcessDormantHeaps();
				return;
			}
		}
		throw std::runtime_error("Staging Page: Can't release a range that is not live.");
	}

	void StagingPage::destroyExcessDormantHeaps()
	{
		/* A heap without live ranges is dormant. Only the dormant heaps above the cap are destroyed, the oldest first. */
		uint32_t dormantCount = 0;
		for(PoolId aliveHeapId : this->aliveHeaps)
		{
			if(this->heaps[aliveHeapId].pendingUploadsCount == 0)
			{
				dormantCount++;
			}
		}
		for(uint32_t i = 0; i < this->aliveHeaps.size() && dormantCount > this->maxDormantHeaps; i++)
		{
			PoolId aliveHeapId = this->aliveHeaps[i];
			if(this->heaps[aliveHeapId].pendingUploadsCount == 0)
			{
				this->freeSpacePerHeap.remove(aliveHeapId);
				this->heaps.remove(aliveHeapId);
				this->aliveHeaps.erase(this->aliveHeaps.begin() + i);
				dormantCount--;
				i--;
			}
		}
	}

	void StagingPage::destroy()
	{
		this->freeSpacePerHeap.clear();
		this->heaps.clear();
		this->aliveHeaps.clear();
		this->liveRanges.clear();
	}

	StagingPage::StagingPage(StagingDirectionEnum direction_, uint64_t upperBoundRangeSize_, uint64_t heapSize_, uint32_t maxDormantHeaps_) : direction(direction_), upperBoundRangeSize(upperBoundRangeSize_), heapSize(heapSize_), maxDormantHeaps(maxDormantHeaps_) {}
}
