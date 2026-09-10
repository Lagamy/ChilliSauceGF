#pragma once
#include "PoolNameless.h"
#include "StagingHeap.h"
#include "FreeSpace.h"
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPULocalPage
{
	uint32_t upperBoundEntrySize;
	uint32_t memoryBlockSize; 
	
	StagingHeap stagingHeap; // Gets pre Sized on creation to handle uploads. If upload is bigger -> it resizes to be bigger. 
	std::vector<MemoryBlock> memoryBlocks;
	std::vector<FreeSpace> freeSpacePerBlock;  
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;
	
	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;

	VkDeviceSize size; // purely for debug porpuses 
	bool created;
	
	void destroy();
	PoolId addBufferInternal(uint32_t memoryId_, VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBufferInternal(uint32_t memoryId_, PoolId bufferId_);
	void addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBuffer(PoolId bufferId_); // Adds free Mem Intervals
	void recreateStagingHeapIfNeeded(uint32_t uploadSize_); 
	void init();

	GPULocalPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_);
};
}
