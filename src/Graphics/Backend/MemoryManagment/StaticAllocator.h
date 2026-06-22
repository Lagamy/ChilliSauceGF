#pragma once
#include "GPUHeap.h"
#include "GPUMemoryEntry.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include <stdexcept>

namespace Graphics
{

// Forward decloration 

struct StaticAllocator { 
	std::array<std::vector<UploadEntry>, 4> uploadEntriesGroups;
	GPUMemoryEntry stagingHeap; 
	GPUHeap gpuHeap;
	bool allocated = false; 

	void recordCMDs(VkCommandBuffer& cmdBuffer_);


	MemoryBlock& getMemoryBlock(); 
	Buffer& getBuffer(UploadTypeEnum uploadType_);
	uint32_t addUpload(const char* name_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_);
	const UploadEntry& getUploadEntry(uint32_t id_, UploadTypeEnum uploadType_);	
	void allocate(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
}; 
}
