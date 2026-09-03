#include "CPUSharedHeap.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void CPUSharedHeap::createStatic(const char* name_)
{	
	this->buffers[{INDEX, 0}].create(this->bufferSizes[{INDEX, 0}], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Index Buffer");
	this->buffers[{VERTEX, 0}].create(this->bufferSizes[{VERTEX, 0}], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	this->buffers[{UNIFORM, 0}].create(this->bufferSizes[{UNIFORM, 0}], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Uniform Buffer");
	this->buffers[{STORAGE, 0}].create(this->bufferSizes[{STORAGE, 0}], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Storage Buffer");
	
	std::array<VkMemoryRequirements, BufferTypesCount> memReqs;

	// Allocate memory
	this->size = 0;
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		memReqs[i] = this->buffers[{i, 0}].memoryReqs; 
		this->bufferOffsets[{i, 0}] = alignUp(this->size, memReqs[i].alignment);
		this->size = this->bufferOffsets[{i, 0}] + memReqs[i].size; 
	}

	// Bind buffer to memory block 
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		if(this->bufferSizes[{i, 0}] != 0)
		{
			vkBindBufferMemory(getMainDevice().logicalDevice, this->buffers[{i, 0}].get(), this->memoryBlocks[0].get(), this->bufferOffsets[{i, 0}]);
		}
	}
	
	this->memoryBlocks[0].create(this->size, this->unit, memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, name_);
}; 


void CPUSharedHeap::destroy()
{
	this->memoryBlocks.clear(); 
	this->buffers.clear();
	this->bufferSizes.clear(); 
	this->bufferOffsets.clear(); 
	this->buffersFreeMemIntervalIdAfterDestruction.clear(); 
}


CPUSharedHeap::CPUSharedHeap(bool isStatic_)
{
	if(isStatic_)
	{
		// Init buffers per type 
		this->buffers.resize(BufferTypesCount);
		this->bufferSizes.resize(BufferTypesCount); 
		this->bufferOffsets.resize(BufferTypesCount);
	}; 
}
}
