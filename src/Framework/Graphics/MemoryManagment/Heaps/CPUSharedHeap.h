#pragma once 
#include "MemoryBlock.h"
#include "Buffer.h" 
#include "Utilities.h"

namespace Graphics
{
struct CPUSharedHeap 
{
	MemoryBlock memoryBlock; // For Index and Vertex buffers 
	std::array<Buffer, BufferTypesCount> buffers;
	std::array<uint32_t, BufferTypesCount> bufferSizes;
	std::array<uint32_t, BufferTypesCount> bufferOffsets;
	// Needed only for GPU_LOCAL
	std::array<uint32_t, BufferTypesCount> bufferFirstByte;  
	bool isGPULocal = false; 

	VkDeviceSize size; 
	StorageUnitEnum unit;
	bool created = false;
	
	void destroy();
	void create(MemoryVisabilityEnum memoryVisability_); 
};
}
