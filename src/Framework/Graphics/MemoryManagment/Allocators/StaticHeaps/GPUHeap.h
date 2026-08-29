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
struct GPUHeap 
{
	std::string name; 
	MemoryBlock memoryBlock;
	
	// Buffers 
	std::array<Buffer, BufferTypesCount> buffersPerType;
	std::array<uint32_t, BufferTypesCount> bufferSizes;
	std::array<uint32_t, BufferTypesCount> bufferOffsets;

	VkDeviceSize size = 0;
	std::vector<Image> images;

	void create(const char* name_);

	uint32_t addBuffer(uint32_t size_, uint32_t offset_); 
	void removeBuffer(uint32_t id_);  
	void destroy();
	GPUHeap();
};
}
