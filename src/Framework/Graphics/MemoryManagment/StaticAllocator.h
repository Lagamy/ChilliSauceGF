#pragma once
#include "MemoryEntry.h"
#include "GPULocalHeap.h"
#include "CPUSharedHeap.h"
#include "StagingHeap.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include "MemoryEntryId.h"
#include "PassId.h"
#include <stdexcept>

namespace Graphics
{
void uploadCMDs(VkCommandBuffer& cmdBuffer_);
// Forward decloration 
struct StaticAllocator { 
	std::vector<MemoryEntry> memoryEntries;
	std::array<std::array<std::vector<uint32_t>, BufferTypesCount>, 2> memoryEntryIdPerBufTypePerMemVisability; 
	
	CPUSharedHeap cpuSharedHeap;
	StagingHeap stagingHeap;
	GPULocalHeap gpuHeap;

	PoolId uploadFinishedSemaphoreId; 
	PoolId uploadFinishedFenceId;
	PassId uploadPassId;

	bool allocated = false;
	bool staticUploadCompleted = false; 

	void create(); 
	void submitUploads();
	void checkUploadsStatus();

	MemoryBlock& getGPUMemoryBlock(); 
	Buffer& getBuffer(BufferTypeEnum uploadType_, MemoryVisabilityEnum memoryVisability_);
	PoolId addEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	void updateEntry(PoolId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t size_); 

	void allocateAndUpload(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
};
} 

