#pragma once
#include "MemoryBlock.h"
#include "Buffer.h"
#include "Image.h"
#include "Pool.h"
#include "Utilities.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPULocalHeap 
{
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
