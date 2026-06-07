// Temp class for convenience. Will later create more robust memory managment system. 

#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 

struct GPUMemoryEntry {
	Buffer buffer; 

	MemoryBlock memoryBlock;
	VkDeviceSize size; 
	StorageUnitEnum unit;
	void* pCpuSharedData; // Pointer to GPU buffered that was mapped to the CPU
	bool cpuShared;
	bool created;
	bool exists = true; 
	
	GPUMemoryEntry(const char* name_, VkDeviceSize size_, StorageUnitEnum unit_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_); 
	~GPUMemoryEntry();
};
