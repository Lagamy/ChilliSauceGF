#pragma once
#include "MemoryBlock.h"
#include "Buffer.h"
#include "Image.h"
#include "Utilities.h"
#include <vulkan/vulkan.h>

namespace Graphics
{
struct GPUHeap 
{
	MemoryBlock memory;
	std::array<Buffer, 2> buffersPerType;
	std::array<uint32_t, 2> bufferSizes;
	std::array<uint32_t, 2> bufferOffsets;
	VkDeviceSize size = 0;
	std::vector<Image> images;

	void create();
	void destroy();
	GPUHeap();
};
}
