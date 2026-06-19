#pragma once
#include "MemoryBlock.h"
#include "Buffer.h"
#include "Image.h"
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPUHeap 
{
	MemoryBlock memory;
	VkDeviceSize size; 
	std::array<Buffer, 4> buffersPerType;
	std::array<uint32_t, 5> bufferSizes; 
	std::vector<Image> images;

	void create();
	void destroy();
};
}
