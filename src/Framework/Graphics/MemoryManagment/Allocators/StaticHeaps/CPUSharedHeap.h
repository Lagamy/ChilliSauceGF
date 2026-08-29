#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 
#include "Utilities.h"

namespace Graphics
{
struct CPUSharedHeap 
{
	MemoryBlock memoryBlock;
	
	std::array<Buffer, BufferTypesCount> buffersPerType;
	std::array<uint32_t, BufferTypesCount> bufferSizes;
	std::array<uint32_t, BufferTypesCount> bufferOffsets;
	VkDeviceSize size; 
	StorageUnitEnum unit;
	bool created;
	
	void destroy();
	void create(const char* name_); 
};
}
