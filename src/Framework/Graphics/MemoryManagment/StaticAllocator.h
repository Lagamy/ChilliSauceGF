#pragma once
#include "UploadEntry.h"
#include "GPUHeap.h"
#include "CPUSharedHeap.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include "UploadId.h"
#include "PassId.h"
#include <stdexcept>

namespace Graphics
{
void staticUploadCMDs(VkCommandBuffer& cmdBuffer_);
// Forward decloration 
struct StaticAllocator { 
	std::array<std::vector<UploadEntry>, BufferTypesCount> gpuUploadEntryGroupPerBufferType;
	std::array<std::vector<UploadEntry>, BufferTypesCount> cpuSharedUploadEntryGroupsPerBufferType;
	CPUSharedHeap cpuSharedHeap; 
	CPUSharedHeap gpuStagingHeap;
	GPUHeap gpuHeap;

	PoolId uploadFinishedSemaphoreId; 
	PoolId uploadFinishedFenceId;
	PassId uploadPassId;

	bool allocated = false;
	bool uploadsInGPU = false; 

	void create(); 
	void submitUploads();
	void checkUploadsStatus();

	MemoryBlock& getMemoryBlock(); 
	Buffer& getBuffer(BufferTypeEnum uploadType_);
	UploadId addUpload(const char* name_, const void* data_, MemoryVisabilityEnum memoryVisability_, VkDeviceSize size_, BufferTypeEnum uploadType_);
	const UploadEntry& getUploadEntry(UploadId id_);	
	void allocateAndUpload(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
}; 
}
