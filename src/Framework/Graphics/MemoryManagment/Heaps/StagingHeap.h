#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 

namespace Graphics
{
struct StagingHeap {
	std::string name; 
	Buffer buffer; 

	MemoryBlock memoryBlock;
	VkDeviceSize size; 
	StorageUnitEnum unit;
	void* pCpuSharedData; // Pointer to GPU buffered that was mapped to the CPU
	bool created;
	
	void destroy();
	void create(const char* name_); 
};
}
