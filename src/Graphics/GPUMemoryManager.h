// Represents resources that are in GPU currently
#pragma once 
#include "CommandBufferBlueprint.h"
#include "GPUMemoryEntry.h"
#include "MemoryBlock.h"
#include "Pool.h"
#include "Semaphore.h"

struct UploadEntry 
{ 
	uint32_t entryId; 
	const void* data; 
	size_t byteAmount = 0; 
	size_t srcStartingByte = 0; 
	size_t dstStartingByte = 0; 

	bool partialUpload;

	UploadEntry(uint32_t entryId_, const void* data_); 
	UploadEntry(uint32_t entryId_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_);
};

struct GPUMemoryManager { 
	Pool<GPUMemoryEntry> memoryEntries = Pool<GPUMemoryEntry>("MemoryEntries"); 
	GPUMemoryEntry uploadHeap; 

	std::vector<UploadEntry> gpuLocalUploadEntries;
	uint32_t addEntry(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_); 
	

	void removeEntry(uint32_t id_); 
	
	

	void upload(uint32_t entryId_, const void* data_); // full upload
	void upload(uint32_t entryId_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_); // partial upload

	void recordCMDs(VkCommandBuffer& cmdBuffer_); // For device local uploads
	
	void submitTransferOps(); 

	GPUMemoryEntry& getEntry(uint32_t id_);

	void create();
	void destroy();
};



