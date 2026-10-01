#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 
#include "Pool.h"

namespace Graphics
{
struct StagingHeap {
	Buffer buffer; 
	MemoryBlock memoryBlock;
	VkDeviceSize size;
	uint64_t pendingUploadsCount = 0; // For initial uploads of Dynamic allocators. If pendingUploadsCount == 0 -> add to pages free list. Check it
	bool isForStaticInit = false; 
	bool created;
	
	void destroy();
	void createStatic();
	void create(VkDeviceSize size_);
	bool wouldNewUploadFit(uint32_t size_);
	void uploadData(const void* data_, uint64_t srcOffset_, size_t ); 
	StagingHeap() = default; 
	StagingHeap(uint64_t size_);
};
} 
