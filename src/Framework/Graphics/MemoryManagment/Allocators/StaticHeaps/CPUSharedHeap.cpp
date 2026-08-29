#include "CPUSharedHeap.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void CPUSharedHeap::create(const char* name_)
{	
	this->buffersPerType[INDEX].create(this->bufferSizes[INDEX], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Index Buffer");
	this->buffersPerType[VERTEX].create(this->bufferSizes[VERTEX], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	this->buffersPerType[UNIFORM].create(this->bufferSizes[UNIFORM], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Uniform Buffer");
	this->buffersPerType[STORAGE].create(this->bufferSizes[STORAGE], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Storage Buffer");
	
	std::array<VkMemoryRequirements, 1> memReqs;

	// Allocate memory
	this->size = 0;
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		memReqs[i] = this->buffersPerType[i].memoryReqs; 
		this->bufferOffsets[i] = alignUp(this->size, memReqs[i].alignment);
		this->size = this->bufferOffsets[i] + memReqs[i].size; 
	}

	// Bind buffer to memory block 
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		if(this->bufferSizes[i] != 0)
		{
			vkBindBufferMemory(getMainDevice().logicalDevice, this->buffersPerType[i].get(), this->memoryBlock.get(), this->bufferOffsets[i]);
		}
	}
	
	this->memoryBlock.create(this->size, this->unit, memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, name_);
}; 


void CPUSharedHeap::destroy()
{
	this->memoryBlock.destroy(); 
	for(uint32_t i = 0; i < this->buffersPerType.size(); i++)
	{
		this->buffersPerType[i].destroy();
		this->bufferSizes[i] = 0;
		this->bufferOffsets[i] = 0; 
	};
}
}
