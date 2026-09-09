#pragma once 
#include "MemoryBlock.h"
#include "PoolNameless.h"
#include "Buffer.h" 
#include "Utilities.h"

namespace Graphics
{
struct CPUSharedHeap 
{
	// Goal: somehow make a map which would answer 1 question: 
	std::vector<MemoryBlock> memoryBlocks;
	std::vector<std::vector<PoolId>> buffersPerMemoryBlock;
	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;


	VkDeviceSize size; 
	StorageUnitEnum unit;
	bool created;
	
	void destroy();
	void createStatic(const char* name_); 
	PoolId addBufferDynamic(uint32_t memoryId_, VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBufferDynamic(uint32_t memoryId_, PoolId bufferId_);
	CPUSharedHeap(bool isStatic_);
};
}
