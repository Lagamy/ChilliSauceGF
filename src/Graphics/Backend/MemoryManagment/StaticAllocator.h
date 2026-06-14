#pragma once
#include "GPUHeap.h"
#include "GPUMemoryEntry.h"
#include "SyncManager.h"
#include "Utilities.h"

struct StaticAllocator { 
	std::array<std::vector<UploadEntry>, 4> uploadEntriesGroups;
	GPUMemoryEntry stagingHeap; 
	GPUHeap gpuHeap;
	PoolId uploadFinishedSemaphore; 
	PoolId uploadFinishedFence;
	bool allocated = false; 

	void recordCMDs(VkCommandBuffer& cmdBuffer_);


	uint32_t addUpload(const char* name_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_);
	UploadEntry& getUploadEntry(uint32_t id_, UploadTypeEnum uploadType_);	
	void allocate(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
}; 
