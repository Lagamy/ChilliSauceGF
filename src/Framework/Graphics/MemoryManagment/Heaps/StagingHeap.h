#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 
#include "Pool.h"
#include "Utilities.h"

namespace Graphics
{
struct StagingHeap {
	Buffer buffer; 
	MemoryBlock memoryBlock;
	VkDeviceSize size;
	uint64_t pendingUploadsCount = 0; // Live ranges in this heap. 0 -> the heap is dormant.
	bool created;
	
	void destroy();
	void create(VkDeviceSize size_, StagingDirectionEnum direction_);
	void uploadData(const void* data_, uint64_t srcOffset_, size_t ); 
	StagingHeap() = default; 
	StagingHeap(uint64_t size_, StagingDirectionEnum direction_);
};
} 
