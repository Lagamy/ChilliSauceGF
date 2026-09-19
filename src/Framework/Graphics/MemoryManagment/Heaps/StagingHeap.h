#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 

namespace Graphics
{
struct StagingHeap {
	Buffer buffer; 

	MemoryBlock memoryBlock;
	VkDeviceSize size; 
	void* pCpuSharedData; // Pointer to GPU buffered that was mapped to the CPU
	bool created;
	bool isDynamic; 
	
	void destroy();
	void createStatic();
	void create(VkDeviceSize size_);  
};
} 
