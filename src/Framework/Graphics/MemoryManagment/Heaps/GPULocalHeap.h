#pragma once
#include "MemoryBlock.h"
#include "Buffer.h"
#include "Image.h"
#include "PoolNameless.h"
#include "Utilities.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPULocalHeap 
{
	std::string name; 
	MemoryBlock memoryBlock;
	
	// Buffers 
	// Static Assumptions 
	// std::array<Buffer, BufferTypesCount> buffersPerType;
	// std::array<uint32_t, BufferTypesCount> bufferSizes;
	// std::array<uint32_t, BufferTypesCount> bufferOffsets;

	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;
	PoolNameless<PoolId> buffersFreeMemIntervalAfterDestruction; // for Dynamic Allocator 

	VkDeviceSize size = 0;
	std::vector<Image> images;

	void createStatic(const char* name_);
	void addBufferDynamic(); 
	void removeBufferDynamic();

	void destroy();
	GPULocalHeap(bool isStatic_);
};
}
