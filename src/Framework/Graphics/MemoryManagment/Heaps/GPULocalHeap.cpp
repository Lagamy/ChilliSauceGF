#include "GPULocalHeap.h"
#include "Api.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void GPULocalHeap::create(MemoryVisabilityEnum memoryVisability_)
{
	VkSharingMode sharingMode = getMemoryManager().queueTransferSync.gpuLocalSharingMode;
	if(this->bufferSizes[INDEX_BUFFER] != 0 || this->bufferSizes[VERTEX_BUFFER] != 0 || this->bufferSizes[UNIFORM_BUFFER] != 0 || this->bufferSizes[STORAGE_BUFFER])
	{
		this->buffers[INDEX_BUFFER].create(this->bufferSizes[INDEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sharingMode, "Index Buffer");
		this->buffers[VERTEX_BUFFER].create(this->bufferSizes[VERTEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sharingMode, "Vertex Buffer");
		this->buffers[UNIFORM_BUFFER].create(this->bufferSizes[UNIFORM_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sharingMode, "Uniform Buffer");
		this->buffers[STORAGE_BUFFER].create(this->bufferSizes[STORAGE_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,sharingMode, "Storage Buffer");

		std::array<VkMemoryRequirements, BufferTypesCount> memReqs; 
	
	
		this->size = 0;
		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			memReqs[i] = this->buffers[i].memoryReqs; 
			this->bufferFirstByte[i] = alignUp(this->size, memReqs[i].alignment);
			this->size = this->bufferFirstByte[i] + memReqs[i].size; 
		}
		this->memoryBlock.createForStaticOrStaging(this->size, memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, GPU_LOCAL, false); 

		for(uint8_t i = 0; i < BufferTypesCount; i++)
		{
			if(this->bufferSizes[i] != 0)
			{
				vkBindBufferMemory(getMainDevice().logicalDevice, this->buffers[i].get(), this->memoryBlock.get(), this->bufferFirstByte[i]);
			}
		}
		this->created = true;
	}
		
};

void GPULocalHeap::destroy()
{
	this->memoryBlock.destroy(); 
	
	for(uint8_t i = 0; i < BufferTypesCount; i++)
	{
		this->buffers[i].destroy(); 
		this->bufferSizes[i] = 0; 
		this->bufferFirstByte[i] = 0; 
	}

	return;
}
}
