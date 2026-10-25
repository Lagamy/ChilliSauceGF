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
			
		this->buffers[INDEX_BUFFER].create(this->bufferSizes[INDEX_BUFFER], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Index Buffer");
		this->buffers[VERTEX_BUFFER].create(this->bufferSizes[VERTEX_BUFFER], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Vertex Buffer");
		this->buffers[UNIFORM_BUFFER].create(this->bufferSizes[UNIFORM_BUFFER],VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Uniform Buffer");
		this->buffers[STORAGE_BUFFER].create(this->bufferSizes[STORAGE_BUFFER], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, "Storage Buffer");
			
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
