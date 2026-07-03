#include "GPUHeap.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
GPUHeap::GPUHeap(){};
void GPUHeap::create()
{
	this->buffersPerType[VERTEX].create(this->bufferSizes[VERTEX], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	this->buffersPerType[INDEX].create(this->bufferSizes[INDEX], VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	// this->buffersPerType[UNIFORM].create(this->bufferSizes[UNIFORM], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	// this->buffersPerType[STORAGE].create(this->bufferSizes[STORAGE], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	
	std::array<VkMemoryRequirements, 2> memReqs; 
	vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffersPerType[VERTEX].get(), &memReqs[VERTEX]);
	vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffersPerType[INDEX].get(), &memReqs[INDEX]);
	// vkGetBufferMemoryRequirements(Demo::renderer.mainDevice.logicalDevice, this->buffersPerType[VERTEX].get(), memReqs[UNIFORM]);
	// vkGetBufferMemoryRequirements(Demo::renderer.mainDevice.logicalDevice, this->buffersPerType[VERTEX].get(), memReqs[STORAGE]);
	
	// this->size = this->bufferSizes[VERTEX] + this->bufferSizes[INDEX] /* + this->bufferSizes[UNIFORM] + this->bufferSizes[STORAGE]*/; 
	this->size = 0;
	for(uint8_t i = 0; i < this->buffersPerType.size(); i++)
	{
		this->bufferOffsets[i] = alignUp(this->size, memReqs[i].alignment);
		this->size += this->bufferOffsets[i] + memReqs[i].size; 
	}
	 
	this->memory.create(this->size, BYTE, memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "GPU Heap");

	for(uint8_t i = 0; i < this->buffersPerType.size(); i++)
	{
		vkBindBufferMemory(getMainDevice().logicalDevice, this->buffersPerType[i].get(), this->memory.get(), this->bufferOffsets[i]);
	}
}

void GPUHeap::destroy()
{
	this->memory.destroy();
	for(uint32_t i = 0; i < this->buffersPerType.size(); i++)
	{
		this->buffersPerType[i].destroy();
		this->bufferSizes[i] = 0;
	}
	for(auto& rImage : this->images)
	{
		rImage.destroy(); 
	}
}
}
