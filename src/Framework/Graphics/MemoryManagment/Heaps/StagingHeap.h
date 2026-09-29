#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 

namespace Graphics
{
struct StagingHeap {
	Buffer buffer; 
	MemoryBlock memoryBlock;
	VkDeviceSize size;
	bool isForStaticInit = false; 
	bool created;
	
	void destroy();
	void createStatic();
	void create(VkDeviceSize size_);
	bool wouldNewUploadFit(uint32_t size_);
	void uploadData(const void* data_, uint64_t srcOffset_, size_t ); 
	StagingHeap() = default; 
	StagingHeap(uint64_t size_); 
	~StagingHeap(); 
};
} 
