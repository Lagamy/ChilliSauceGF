// Temp class for convenience. Will later create more robust memory managment system. 

#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 

struct PhysicalGPUBuffer {
	std::string name; 
	Buffer buffer; 

	MemoryBlock memoryBlock;
	size_t size; 
	void* pCpuSharedData; // Pointer to GPU buffered that was mapped to the CPU
	bool cpuShared;
	bool created;
	bool exists = true; 
	// Those are in use only if isLocalHost = true; 
	Buffer stagingBuffer; 
	MemoryBlock stagingMemoryBlock;
	
	PhysicalGPUBuffer(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_); 
	~PhysicalGPUBuffer();

	void recreate(); 
};
