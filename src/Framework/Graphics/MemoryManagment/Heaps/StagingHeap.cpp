#include "StagingHeap.h"
#include "Api.h"

namespace Graphics
{
void StagingHeap::create(const char* name_)
{
    this->name = name_; 
	this->unit = BYTE;
	std::array<VkMemoryRequirements, 1> memRequirement;
	this->buffer.create(this->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, "Staging Buffer"); 

	// Allocate memory
	vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffer.get(), &memRequirement[0]);
	this->memoryBlock.create(this->size, this->unit, memRequirement, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, name_);

	// Bind buffer to memory block 
	vkBindBufferMemory(getMainDevice().logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
}; 


void StagingHeap::destroy()
{
	this->memoryBlock.destroy(); 
	this->buffer.destroy();
}
}