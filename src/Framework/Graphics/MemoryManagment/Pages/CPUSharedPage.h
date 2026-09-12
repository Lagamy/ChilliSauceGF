#pragma once 
#include "PoolId.h"
#include "PoolNameless.h"
#include "MemoryBlock.h"
#include "Buffer.h"
#include "Api.h"
#include "Utilities.h"
#include "FreeSpace.h"
#include <unordered_map>

namespace Graphics
{
struct CPUSharedPage
{
	uint32_t upperBoundEntrySize;
	uint32_t memoryBlockSize; 
	
	std::vector<MemoryBlock> memoryBlocks;
	std::vector<FreeSpace> freeSpacePerBlock;  
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;
	
	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;

	VkDeviceSize size; // purely for debug porpuses 
	bool created;
	
	void destroy();
	PoolId addBufferInternal(uint32_t memoryId_, VkDeviceSize memoryOffset_, VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBufferInternal(PoolId bufferId_);
	void addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeBuffer(PoolId bufferId_); // Adds free Mem Intervals
	void init();

	CPUSharedPage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_);
};
}