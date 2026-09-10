#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 
#include "Utilities.h"

namespace Graphics
{
struct CPUSharedHeap 
{
	// Goal: somehow make a map which would answer 1 question: 
	MemoryBlock memoryBlock;
	std::array<Buffer, BufferTypesCount> buffers;
	std::array<uint32_t, BufferTypesCount> bufferSizes;
	std::array<uint32_t, BufferTypesCount> bufferOffsets;

	VkDeviceSize size; 
	StorageUnitEnum unit;
	bool created;
	
	void destroy();
	void create(); 
};
}
