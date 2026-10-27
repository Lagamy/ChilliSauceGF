#include "StagingAllocator.h"
#include "Api.h"
#include <algorithm>
#include <format>
#include <stdexcept>

namespace Graphics
{
void StagingAllocator::addPage(StagingDirectionEnum direction_, uint32_t upperBoundForRangeSize_, StorageUnitEnum upperBoundUnit_, uint32_t heapSize_, StorageUnitEnum heapSizeUnit_, uint32_t maxDormantHeaps_)
{
	VkDeviceSize upperBoundRangeSizeInBytes = toBytes(upperBoundForRangeSize_, upperBoundUnit_);
	VkDeviceSize heapSizeInBytes = toBytes(heapSize_, heapSizeUnit_);

	#ifdef ENGINE_DEBUG
		if(this->initialized)
		{
			throw std::runtime_error("Staging Allocator: Can't add Page after allocator was created.");
		}
		if(upperBoundRangeSizeInBytes > heapSizeInBytes)
		{
			throw std::runtime_error("Staging Allocator: Can't add Page where upper bound range size is bigger than single staging heap size.");
		}
	#endif

	this->pageInfos[direction_].emplace_back(upperBoundRangeSizeInBytes, heapSizeInBytes, maxDormantHeaps_);
}

void StagingAllocator::init()
{
	for(uint8_t direction = 0; direction < StagingDirectionsCount; direction++)
	{
		std::sort(this->pageInfos[direction].begin(), this->pageInfos[direction].end());
		this->pages[direction].reserve(this->pageInfos[direction].size());
		for(const StagingPageInfo& rInfo : this->pageInfos[direction])
		{
			this->pages[direction].emplace_back(static_cast<StagingDirectionEnum>(direction), rInfo.upperBoundRangeSize, rInfo.heapSize, rInfo.maxDormantHeaps);
		}
	}
	this->initialized = true;
}

StagingData StagingAllocator::allocate(StagingDirectionEnum direction_, uint64_t size_)
{
	/*
		vkInvalidateMappedMemoryRanges needs the offset and size of the invalidated range to be multiples of nonCoherentAtomSize (VUID-VkMappedMemoryRange-size-01390, -offset-00687).
		READBACK range sizes are rounded up to it, even though the heap may turn out coherent, since the range is sized before a heap is picked. Ranges are placed from the start of a heap, so every offset becomes a multiple of it too.
	*/
	if(direction_ == READBACK)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(getMainDevice().physicalDevice, &deviceProperties);
		size_ = alignUp(size_, deviceProperties.limits.nonCoherentAtomSize);
	}

	/* The first page whose upper bound fits the range serves it, so small writes never share heaps with big ones. */
	for(uint32_t i = 0; i < this->pages[direction_].size(); i++)
	{
		if(size_ <= this->pages[direction_][i].upperBoundRangeSize)
		{
			StagingRange range = this->pages[direction_][i].allocate(size_);
			StagingData stagingData = {};
			stagingData.heapId = range.heapId;
			stagingData.size = size_;
			stagingData.stagingOffset = range.firstByte;
			stagingData.stagingPageId = i;
			stagingData.direction = direction_;
			return stagingData;
		}
	}
	throw std::runtime_error(std::format("Staging Allocator: unable to find {} page with upper bound big enough for a range of {} bytes.", direction_ == UPLOAD ? "upload" : "readback", size_));
}

void StagingAllocator::tagBatch(const StagingData& rStagingData_, uint64_t batchId_)
{
	this->pages[rStagingData_.direction][rStagingData_.stagingPageId].tagBatch(rStagingData_.heapId, rStagingData_.stagingOffset, batchId_);
}

StagingHeap& StagingAllocator::getHeap(const StagingData& rStagingData_)
{
	return this->pages[rStagingData_.direction][rStagingData_.stagingPageId].heaps[rStagingData_.heapId];
}

void StagingAllocator::releaseBatch(uint64_t batchId_)
{
	for(StagingPage& rPage : this->pages[UPLOAD])
	{
		rPage.releaseBatch(batchId_);
	}
}

void StagingAllocator::releaseReadback(const StagingData& rStagingData_)
{
	this->pages[READBACK][rStagingData_.stagingPageId].release(rStagingData_.heapId, rStagingData_.stagingOffset);
}

void StagingAllocator::deallocate()
{
	for(uint8_t direction = 0; direction < StagingDirectionsCount; direction++)
	{
		for(StagingPage& rPage : this->pages[direction])
		{
			rPage.destroy();
		}
		this->pages[direction].clear();
	}
}
}
