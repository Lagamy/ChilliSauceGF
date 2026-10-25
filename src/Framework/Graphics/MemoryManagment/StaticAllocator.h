#pragma once
#include "CPUSharedHeap.h"
#include "GPULocalHeap.h"
#include "MemoryEntry.h"
#include "GPULocalHeap.h"
#include "CPUSharedHeap.h"
#include "MemoryBlock.h"
#include "Utilities.h"
#include "MemoryEntryId.h"
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace Graphics
{
// Forward decloration 
struct StaticAllocator { 
	std::vector<MemoryEntry> memoryEntries;
	std::array<std::array<std::vector<uint32_t>, BufferTypesCount>, 2> memoryEntryIdPerBuffTypePerMemVisability;

	CPUSharedHeap cpuSharedHeap;
	GPULocalHeap gpuLocalHeap;

	bool allocated = false;

	MemoryBlock& getGPUMemoryBlock(); 
	Buffer& getBuffer(BufferTypeEnum uploadType_, MemoryVisabilityEnum memoryVisability_);
	PoolId addEntry(const char* name_, const void* data_, VkDeviceSize size_, MemoryVisabilityEnum memoryVisability_, BufferTypeEnum uploadType_);
	void updateEntry(PoolId entryId_, const void* data_, size_t entryOffset_, size_t srcOffset_, size_t size_); 

	void allocateAndUpload(); // Run only when you added all UploadEntries for that scene/demo 
	void deallocate();
};
} 

