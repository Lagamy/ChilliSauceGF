// Represents resources that are in GPU currently
#pragma once 
#include "CommandBufferBlueprint.h"
#include "GPUMemoryEntry.h"
#include "MemoryBlock.h"
#include "Pool.h"
#include "Semaphore.h"

struct UploadEntry 
{ 
	PoolId entryId; 
	const void* data; 
	size_t byteAmount = 0; 
	size_t heapStartinByte = 0; 
	size_t dstStartingByte = 0; 

	bool partialUpload;

	UploadEntry(PoolId entryId_, const void* data_); 
	UploadEntry(PoolId entryId_, const void* data_, size_t byteAmount_, size_t heapStartinByte_, size_t dstStartingbyte_);
};

struct UploadHeap 
{
	size_t lastByte = 0;
	GPUMemoryEntry memoryEntry; 
};

struct GPUMemoryManager { 
	Pool<GPUMemoryEntry> memoryEntries = Pool<GPUMemoryEntry>("MemoryEntries"); 
	UploadHeap uploadHeap; 

	std::vector<UploadEntry> gpuLocalUploadEntries;
	PoolId addEntry(const char* name_, VkDeviceSize size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_); 
	

	void removeEntry(PoolId id_); 
	

	void upload(PoolId entryId_, const void* data_); // full upload
	void upload(PoolId entryId_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_); // partial upload

	void recordCMDs(VkCommandBuffer& cmdBuffer_); // For device local uploads
	
	void submitTransferOps(); 

	GPUMemoryEntry& getEntry(PoolId id_);

	void create();
	void destroy();
};



