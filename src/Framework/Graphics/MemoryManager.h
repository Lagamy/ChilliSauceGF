// Represents resources that are in GPU currently
#pragma once 
#include "PassId.h"
#include "Pass.h"
#include "Semaphore.h"
#include "StagingHeap.h"
#include "MemoryBlock.h"
#include "Pool.h"
#include "SyncManager.h"
#include "Image.h"
#include "StaticAllocator.h"
#include "Utilities.h"
#include <functional>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
struct UpdateEntry 
{ 
	uint32_t entryId; 
	BufferTypeEnum uploadType; 
	size_t byteAmount = 0; 
	size_t heapStartingByte = 0; 
	size_t dstStartingByte = 0; 
	AllocatorTypeEnum allocatorType; 

	bool partialUpdate;

	UpdateEntry(uint32_t entryId_, BufferTypeEnum uploadType_, const void* data_, size_t heapStartingByte_); 
	UpdateEntry(uint32_t entryId_, BufferTypeEnum uploadType_, const void* data_, size_t heapStartingByte_, size_t byteAmount_, size_t srcStartingbyte_);
};


struct MemoryManager 
{ 
	StaticAllocator staticAllocator;
	std::vector<UpdateEntry> updateEntries; 
	bool updateNeeded = false;
	void submitTransferCmds();
	PoolId updateFinishedSemaphoreId; 
	PoolId updateFinishedFenceId; 
	const uint8_t allocatorsCount = 1; 

	bool isUploadInGPU(UploadId uploadId_);
	void checkUploadsStatus(); 
	
	void addUpdate(PoolId entryId_, const void* data_); // full upload
	void addUpdate(PoolId entryId_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_); // partial upload

	// For device local uploads
	void submitUpdateCmdsIfNeeded();

	void create();
	void destroy();
};
}
