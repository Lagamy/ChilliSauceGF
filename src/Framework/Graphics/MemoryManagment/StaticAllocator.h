#pragma once
#include "UploadEntryGroup.h"
#include "GPULocalHeap.h"
#include "CPUSharedHeap.h"
#include "StagingHeap.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include "UploadId.h"
#include "PassId.h"
#include <stdexcept>

namespace Graphics
{
void uploadCMDs(VkCommandBuffer& cmdBuffer_);
// Forward decloration 
struct StaticAllocator { 
	std::array<UploadEntryGroup, 2> uploadEntryGroupPerMemVisability;
	CPUSharedHeap cpuSharedHeap = CPUSharedHeap(true); 
	StagingHeap stagingHeap;
	GPULocalHeap gpuHeap = GPULocalHeap(true);

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
	UploadId addEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	void updateEntry(UploadId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t byteAmmount_); 

	const UploadEntry& getEntry(UploadId id_);	
	void allocateAndUpload(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
};
} 

