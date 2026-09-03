#pragma once 
#include "MemoryBlock.h"
#include "PoolNameless.h"
#include "Buffer.h" 
#include "Utilities.h"

namespace Graphics
{
struct CPUSharedHeap 
{
	std::vector<MemoryBlock> memoryBlocks;
	
	// std::array<Buffer, BufferTypesCount> buffersPerType;
	// std::array<uint32_t, BufferTypesCount> bufferSizes;
	// std::array<uint32_t, BufferTypesCount> bufferOffsets;
	
	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;
	PoolNameless<PoolId> buffersFreeMemIntervalIdAfterDestruction; // for Dynamic Allocator 


	VkDeviceSize size; 
	StorageUnitEnum unit;
	bool created;
	
	void destroy();
	void createStatic(const char* name_); 
	void addOrExtendBufferDynamic();
	void removeOrShrinkBufferDynamic();
	CPUSharedHeap(bool isStatic_);
};
}
