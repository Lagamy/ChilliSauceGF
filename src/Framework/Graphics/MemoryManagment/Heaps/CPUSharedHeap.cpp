#include "CPUSharedHeap.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void CPUSharedHeap::create(MemoryVisabilityEnum memoryVisability_)
{
	if(this->bufferSizes[INDEX_BUFFER] != 0 || this->bufferSizes[VERTEX_BUFFER] != 0 || this->bufferSizes[UNIFORM_BUFFER] != 0 || this->bufferSizes[STORAGE_BUFFER])
	{
			
		this->buffers[INDEX_BUFFER].create("Index Buffer", this->bufferSizes[INDEX_BUFFER], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
		this->buffers[VERTEX_BUFFER].create("Vertex Buffer", this->bufferSizes[VERTEX_BUFFER], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
		this->buffers[UNIFORM_BUFFER].create("Uniform Buffer", this->bufferSizes[UNIFORM_BUFFER],VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
		this->buffers[STORAGE_BUFFER].create("Storage Buffer", this->bufferSizes[STORAGE_BUFFER], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
			
		std::array<VkMemoryRequirements, BufferTypesCount> memReqs;	

		// Allocate memory
		this->size = 0;
		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			memReqs[i] = this->buffers[i].memoryReqs; 
			this->bufferOffsets[i] = alignUp(this->size, memReqs[i].alignment);
			this->size = this->bufferOffsets[i] + memReqs[i].size; 
		}

			
		this->memoryBlock.createForStaticOrStaging(this->size, memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, CPU_SHARED, false);
	
		// Bind buffer to memory block
		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			if(this->bufferSizes[i] != 0)
			{
				vkBindBufferMemory(getMainDevice().logicalDevice, this->buffers[i].get(), this->memoryBlock.get(), this->bufferOffsets[i]);
			}
		}

		this->created = true; 
	}
};

void CPUSharedHeap::destroy()
{
	this->memoryBlock.destroy(); 
	
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		this->buffers[i].destroy(); 
		this->bufferSizes[i] = 0; 
		this->bufferOffsets[i] = 0; 
	}

	
}
}
