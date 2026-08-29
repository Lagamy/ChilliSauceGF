#pragma once
#include "UploadEntryGroup.h"
#include "GPUHeap.h"
#include "CPUSharedHeap.h"
#include "StagingHeap.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include "UploadId.h"
#include "PassId.h"
#include "UpdateEntry.h"
#include <stdexcept>

namespace Graphics
{
void uploadCMDs(VkCommandBuffer& cmdBuffer_);
// Forward decloration 
struct StaticAllocator { 
	std::array<UploadEntryGroup, 2> uploadEntryGroupPerMemVisability;
	CPUSharedHeap cpuSharedHeap; 
	StagingHeap stagingHeap;
	GPUHeap gpuHeap;

	std::vector<UpdateEntry> updateEntries; 

	PoolId uploadFinishedSemaphoreId; 
	PoolId uploadFinishedFenceId;
	PassId uploadPassId;

	bool allocated = false;
	bool uploadsInGPU = false; 

	void create(); 
	void submitUploads();
	void checkUploadsStatus();

	MemoryBlock& getGPUMemoryBlock(); 
	Buffer& getBuffer(BufferTypeEnum uploadType_);
	UploadId addUpload(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	const UploadEntry& getUploadEntry(UploadId id_);	
	void allocateAndUpload(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
};
} 

