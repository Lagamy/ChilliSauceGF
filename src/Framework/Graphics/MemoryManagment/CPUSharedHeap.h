// Temp class for convenience. Will later create more robust memory managment system. 

#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 

namespace Graphics
{
struct CPUSharedHeap {
	Buffer buffer; 

	MemoryBlock memoryBlock;
	VkDeviceSize size; 
	StorageUnitEnum unit;
	void* pCpuSharedData; // Pointer to GPU buffered that was mapped to the CPU
	bool created;
	
	void destroy();
	void create(const char* name_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_); 
};
}
