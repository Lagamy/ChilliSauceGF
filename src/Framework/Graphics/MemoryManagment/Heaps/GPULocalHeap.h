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
	PoolNameless<Buffer> buffers;
	PoolNameless<uint32_t> bufferSizes;
	PoolNameless<uint32_t> bufferOffsets;

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
