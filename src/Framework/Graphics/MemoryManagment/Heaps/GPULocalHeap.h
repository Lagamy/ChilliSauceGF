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
	std::vector<MemoryBlock> memoryBlocks; 
	
	// Buffers 
	// Static Assumptions 
	// std::array<Buffer, BufferTypesCount> buffersPerType;
	// std::array<uint32_t, BufferTypesCount> bufferSizes;
	// std::array<uint32_t, BufferTypesCount> bufferOffsets;

	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;
	// for Dynamic Allocator
	PoolNameless<PoolId> buffersFreeMemIntervalIdAfterDestruction;
	std::vector<uint32_t> aliveBuffers;

	VkDeviceSize size = 0;
	std::vector<Image> images;

	void createStatic(const char* name_);
	void addOrExtendBufferDynamic(VkDeviceSize size_, BufferTypeEnum bufferType_);
	void removeOrShrinkBufferDynamic(VkDeviceSize size_, BufferTypeEnum bufferType_);
	void createOrExtendMemoryBlockDynamic(VkDeviceSize byteAmount_); 
	void destroy();
	GPULocalHeap(bool isStatic_);
};
}
