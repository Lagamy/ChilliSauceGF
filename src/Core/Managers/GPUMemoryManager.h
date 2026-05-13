// Represents resources that are in GPU currently
#pragma once 
#include "PhysicalGPUBuffer.h"
#include "Pool.h"

struct GPUMemoryManager { 
	Pool<PhysicalGPUBuffer> memoryEntries = Pool<PhysicalGPUBuffer>("MemoryEntries"); 

	uint32_t addEntry(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_); 
	
	void removeEntry(uint32_t id_); 
	void upload(uint32_t entryId_, const void* data_, VkQueue queue_); // full upload
	void upload(uint32_t entryId_, const void* data_, VkQueue queue_, size_t byteAmount_, size_t srcStartingByte_, size_t dstStartingByte_); // partial upload

	PhysicalGPUBuffer& getEntry(uint32_t id_);


	
};
