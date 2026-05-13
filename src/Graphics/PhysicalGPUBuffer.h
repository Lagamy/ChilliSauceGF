// Temp class for convenience. Will later create more robust memory managment system. 

#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 
#include "CommandPool.h"

struct PhysicalGPUBuffer {
	std::string name; 
	Buffer buffer; 

	MemoryBlock memoryBlock;
	size_t size; 
	void* pCpuSharedData; // Pointer to GPU buffered that was mapped to the CPU
	bool cpuShared;
	bool created;

	// Those are in use only if isLocalHost = true; 
	Buffer stagingBuffer; 
	MemoryBlock stagingMemoryBlock;
	
	PhysicalGPUBuffer(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_); 
	~PhysicalGPUBuffer();

	// add uploads and partial upload
	void recreate(); 
	void upload(const void* data_, VkQueue queue_, CommandPool& rCommandPool_); // full upload
	void upload(const void* data_, VkQueue queue_, CommandPool& rCommandPool_, size_t byteAmount_, size_t srcStartingByte_, size_t dstStartingByte_); // partial upload 
};
