#pragma once
#include "UploadEntry.h"
#include "GPUHeap.h"
#include "StagingHeap.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include "UploadId.h"
#include <stdexcept>

namespace Graphics
{

// Forward decloration 

struct StaticAllocator { 
	std::array<std::vector<UploadEntry>, 2> uploadEntriesGroups;
	StagingHeap stagingHeap; 
	GPUHeap gpuHeap;
	bool allocated = false; 

	void recordCMDs(VkCommandBuffer& cmdBuffer_);


	MemoryBlock& getMemoryBlock(); 
	Buffer& getBuffer(UploadTypeEnum uploadType_);
	UploadId addUpload(const char* name_, const void* data_, VkDeviceSize size_, UploadTypeEnum uploadType_);
	const UploadEntry& getUploadEntry(UploadId id_);	
	void allocate(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
}; 
}
