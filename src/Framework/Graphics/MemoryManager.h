// Represents resources that are in GPU currently
#pragma once 
#include "DynamicAllocator.h"
#include "StaticAllocator.h"
#include "UploadEntry.h"
#include "Utilities.h"
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
