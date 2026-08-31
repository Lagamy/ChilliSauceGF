// Represents resources that are in GPU currently
#pragma once 
#include "GPULocalHeap.h"
#include "PassId.h"
#include "Pass.h"
#include "Semaphore.h"
#include "CPUSharedHeap.h"
#include "MemoryBlock.h"
#include "Pool.h"
#include "StagingHeap.h"
#include "SyncManager.h"
#include "Image.h"
#include "DynamicAllocator.h"
#include "StaticAllocator.h"
#include "UploadEntry.h"
#include "UploadEntryGroup.h"
#include "Utilities.h"
#include <functional>
#include <vulkan/vulkan_core.h>

namespace Graphics
{

struct MemoryManager 
{
	StaticAllocator uiAllocator;    
	StaticAllocator staticAllocator;
	DynamicAllocator dynamicAllocator; 

	void submitTransferCmds();
	void checkUploadsStatus(); 
	bool isUploadInGPU(UploadId uploadId_);
	
	UploadEntry addUploadEntry(const char* name_, const void* data_, VkDeviceSize size_, AllocatorTypeEnum allocatorType_, BufferTypeEnum bufferType_); 

	// Upload Entries
	void submitUploadsUI();
	void submitUploadsStatic();
	void submitUploadsDynamic();

	void addUpdate(UploadEntry uploadEntry_, const void* data_); // full upload
	void addUpdate(UploadEntry uploadEntry_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_); // partial upload

	// For device local uploads
	void submitUpdateCmdsIfNeeded();

	void setup();
	void update(); 
	void destroy();
};
}
