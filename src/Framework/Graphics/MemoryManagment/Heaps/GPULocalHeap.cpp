#include "GPULocalHeap.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void GPULocalHeap::createStatic(const char* name_)
{
	this->name = name_; 
	
	this->buffers[{INDEX, 0}].create(this->bufferSizes[{INDEX, 0}], VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Index Buffer");
	this->buffers[{VERTEX, 0}].create(this->bufferSizes[{VERTEX, 0}], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
	this->buffers[{UNIFORM, 0}].create(this->bufferSizes[{UNIFORM, 0}], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Uniform Buffer");
	this->buffers[{STORAGE, 0}].create(this->bufferSizes[{STORAGE, 0}], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, "Storage Buffer");

	std::array<VkMemoryRequirements, BufferTypesCount> memReqs; 
	
	
	this->size = 0;
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		memReqs[i] = this->buffers[{i, 0}].memoryReqs; 
		this->bufferOffsets[{i, 0}] = alignUp(this->size, memReqs[i].alignment);
		this->size = this->bufferOffsets[{i, 0}] + memReqs[i].size; 
	}
	 
	this->memoryBlock.create(this->size, BYTE, memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "GPU Heap");

	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		vkBindBufferMemory(getMainDevice().logicalDevice, this->buffers[{i, 0}].get(), this->memoryBlock.get(), this->bufferOffsets[{i, 0}]);
	}
}

void GPULocalHeap::destroy()
{
	this->memoryBlock.destroy();
	this->buffers.clear();
	this->bufferSizes.clear(); 
	this->bufferOffsets.clear(); 
	this->buffersFreeMemIntervalAfterDestruction.clear();
	for(auto& rImage : this->images)
	{
		rImage.destroy(); 
	}
}

GPULocalHeap::GPULocalHeap(bool isStatic_)
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
