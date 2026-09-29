#pragma once 
#include "Pool.h"
#include "MemoryBlock.h"
#include "FreeSpace.h"
#include "Buffer.h"
#include "PoolId.h"
#include "Utilities.h"

namespace Graphics 
{

struct BufferCreationResult 
{
	PoolId memoryBlockId; 
	PoolId bufferId; 
};

struct BasePage 
{
    uint32_t upperBoundEntrySize;
	uint32_t memoryBlockSize;
    uint32_t memoryProperties;  
    MemoryVisabilityEnum memoryVisability; 
	
	Pool<MemoryBlock> memoryBlocks;
	Pool<FreeSpace> freeSpacePerBlock;  
	std::vector<PoolId> aliveMemoryBlocks;  
	
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;
	
	Pool<Buffer> buffers;
	Pool<uint32_t> buffersFirstByteOffset;

	VkDeviceSize size; // purely for debug porpuses 
	bool created;
	
	void destroy();
	PoolId addBufferInternal(PoolId memoryId_, VkDeviceSize memoryFirstByteOffset_, VkDeviceSize size_, BufferTypeEnum bufferType_, const char* uploadName_);
	void removeBufferInternal(PoolId bufferId_);
	BufferCreationResult addBuffer(VkDeviceSize size_, BufferTypeEnum bufferType_, const char* uploadName_);
	void removeBuffer(PoolId bufferId_, PoolId memoryBlockId_); // Adds free Mem Intervals
	void removeMemoryBlock(PoolId memoryBlockId_); 
	void init();

	BasePage(uint32_t upperBoundEntrySize_, uint32_t memoryBlockSize_, MemoryVisabilityEnum memoryVisability_);    
};
}